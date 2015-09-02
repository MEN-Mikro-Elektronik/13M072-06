/****************************************************************************
 ************                                                    ************
 ************                 M 7 2 _ C O U N T                  ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *        $Date: 2010/04/20 15:04:42 $
 *    $Revision: 1.9 $
 *
 *  Description: Configure and read M72 counter channel
 *
 *               Universal tool for configuring M72 and make measurements
 *               with all counter modes.
 *               The modules interrupt is enabled trough the M_MK_IRQ_ENABLE
 *               setstat call. 
 *
 *     Required: usr_oss.l usr_utl.l
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_count.c,v $
 * Revision 1.9  2010/04/20 15:04:42  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.8  2006/09/27 18:58:11  ts
 * changed return type of main() to int
 *
 * Revision 1.7  2004/08/30 12:19:15  dpfeuffer
 * minor modifications for MDIS4/2004 conformity
 *
 * Revision 1.6  1999/08/06 15:07:26  Franke
 * cosmetics
 *
 * Revision 1.5  1999/08/06 10:30:08  Schoberl
 * added headers stdlib.h, string.h
 *
 * Revision 1.4  1999/08/06 09:19:11  Schoberl
 * cosmetics
 * functions static
 * adapted to current driver
 * added:
 * - count mode 6,7,9: counter preload in loop mode
 * - signal for XIN2
 * - interrupt per channel
 * removed:
 * - recursive abort when error occures while disabling interrupt
 *
 * Revision 1.3  1998/11/02 17:10:37  see
 * casts added
 * V1.1
 *
 * Revision 1.2  1998/10/30 15:25:30  Schmidt
 * ReadySig, CompSig, CybwSig, LbreakSig are now u_int32
 * SetGetStat(): maxInfo now int32
 * main(): access to returned NULL-ptr. from UTL_TSTOPT() prevented
 *
 * Revision 1.1  1998/10/23 16:31:10  see
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static const char RCSid[]="$Id: m72_count.c,v 1.9 2010/04/20 15:04:42 amorbach Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m72_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/* info prefix */
#define PRE "                 "

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/* general */
static u_int32 G_Verbose;
static u_int32 G_Signal = 0;

/* M72 channels */
static char *G_ChanInfo[] = {
	"Counter A (0)",
	"Counter B (1)",
	"Counter C (2)",
	"Counter D (3)",
	NULL
};

/* M72 counter modes */
static char *G_CntModeInfo[] = {
	"no count (halted)",
	"single count",
	"1x quadrature count",
	"2x quadrature count",
	"4x quadrature count",
	"frequency measurement",
	"pulse width high measurement",
	"pulse width low measurement",
	"no valid value",
	"period measurement",
	"timer mode",
	NULL
};

/* M72 counter preload/clear conditions */
static char *G_PreClrCondInfo[] = {
	"no condition (disabled)",
	"at xIN2 rising edge",
	"now (immediately)",
	"at comparator match",
	NULL
};

/* M72 counter store conditions */
static char *G_StoreCondInfo[] = {
	"no condition (disabled)",
	"now (immediately)",
	"at xIN2 rising edge",
	NULL
};

/* M72 comparator interrupt condition */
static char *G_CompIrqInfo[] = {
	"no condition (disabled)",
	"if counter < COMPA",
	"if counter > COMPA",
	"if counter = COMPA",
	"if COMPA < counter < COMPB",
	"if COMPA > counter or COMPB < counter", 
	NULL
};

/* M72 carry/borrow interrupt condition */
static char *G_CybwIrqInfo[] = {
	"no condition (disabled)",
	"if carry",
	"if borrow",
	"if carry or borrow",
	NULL
};

/* M72 read mode flags */
static char *G_RdModeInfo[] = {
	"read latched value",
	"wait for ready irq before read",
	"force counter latch before read",
	NULL
};

/* M72 write mode flags */
static char *G_WrModeInfo[] = {
	"write preload value",
	"no valid value",                
	"force counter load after write",
	NULL
};

/* M72 timer start conditions */
static char *G_TimerStartInfo[] = {
	"at xIN2 rising edge",
	"now (immediately)",
	NULL
};

/* general enable */
static char *G_EnbIrqInfo[] = {
	"disable",
	"enable",
	NULL
};


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void __MAPILIB SigHandler( u_int32 sigCode );
static void usage(int32 moreHelp);
static void PrintError(char *info);
static void PrintInfo(char *prefix, u_int32 prval, char **info);
static int32 SetGetStat(MDIS_PATH path, int32 setstat,	int32 *valueP, int32 code,
				 char *name, char *desc, char **info);

/********************************* SigHandler *******************************
 *
 *  Description: Signal handler
 *			   
 *---------------------------------------------------------------------------
 *  Input......: sigCode	signal code received
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/

static void __MAPILIB SigHandler( u_int32 sigCode )
{
	G_Signal = sigCode;
}

/********************************* usage ************************************
 *
 *  Description: Print program usage
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void usage(int32 moreHelp)
{
	printf("Usage: m72_count [<opts>] <device> [<opts>]\n");
	printf("Function: Configure and read M72 counter channel\n");
	printf("Options:\n");
	printf("    device       device name                            [none]\n");
	printf("    -c=<chan>    channel number (0..3)                  [none]\n");
	printf("    -R=<mode>    mode for read  calls                   [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_RdModeInfo);
	printf("    -W=<mode>    mode for write calls                   [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_WrModeInfo);
	printf("    -m=<mode>    counter mode                           [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_CntModeInfo);
	printf("    -p=<cond>    counter preload condition              [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_PreClrCondInfo);
	printf("    -e=<cond>    counter clear   condition              [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_PreClrCondInfo);
	printf("    -s=<cond>    counter store   condition              [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_StoreCondInfo);
	printf("    -o=<comp>    comparator   irq condition             [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_CompIrqInfo);
	printf("    -y=<comp>    carry/borrow irq condition             [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_CybwIrqInfo);
	printf("    -k=<enb>     line-break   irq enable                [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_EnbIrqInfo);
	printf("    -r=<enb>     xIN2 edge    irq enable                [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_EnbIrqInfo);
	printf("    -x=<enb>     per channel  irq enable                [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_EnbIrqInfo);
	printf("    -t=<enb>     timer start condition (timer mode)     [none]\n");
	if (moreHelp) PrintInfo(PRE, TRUE, G_TimerStartInfo);
	printf("    -a=<val>     comparator A value (hex)            	[none]\n");
	printf("    -b=<val>     comparator B value (hex)            	[none]\n");
	printf("    -u=<val>     counter preload value (hex)            [none]\n");
	printf("    -i=<mode>    output signal mode (hex)            	[none]\n");
	printf("    -g=<set>     output signal setting (hex)            [none]\n");
	printf("    -f=<conf>    selftest configuration (hex)           [none]\n");
	printf("    -1=<sigcode> install ready        signal (0=none)   [none]\n");
	printf("    -2=<sigcode> install comparator   signal (0=none)   [none]\n");
	printf("    -3=<sigcode> install carry/borrow signal (0=none)   [none]\n");
	printf("    -4=<sigcode> install line-break   signal (0=none)   [none]\n");
	printf("    -5=<sigcode> install XIN2         signal (0=none)   [none]\n");
	printf("        sigcode..see usr_os.h	\n");
	printf("    -l           loop mode                              [OFF]\n");
	printf("    -d=<msec>    loop mode delay (0=none) [msec]        [200]\n");
	printf("    -v           verbose (print current values)         [OFF]\n");
	printf("    -n           do not read counter                    [OFF]\n");
	printf("    -h           print detailed values for all options  \n");
	printf("    -<opt>=?     print detailed values for option <opt> \n");
	printf("    -?           print this help\n");
	printf("(c) 1999 by MEN mikro elektronik GmbH, %s\n\n", RCSid );
}

/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH path=0;
	int32 valCount,ret,error,n;
	int32 chan,readMode,writeMode,cntMode,preCond,clrCond,storeCond;
	int32 compIrq,cybwIrq,timerStart,dontRead,valOutMode,valOutSet,valSelftest;
	int32 setOutMode=FALSE,setOutSet=FALSE,setSelftest=FALSE, chanIrq;
	int32 lbreakIrq,xin2Irq,loopMode,loopDelay,valCompA,valCompB,valPreload;
	int32 setCompA=FALSE,setCompB=FALSE,setPreload=FALSE;
	u_int32 intEn = FALSE;
	u_int32 ReadySig, CompSig, CybwSig, LbreakSig, Xin2Sig;

	char *device,*str,*errstr,errbuf[40];

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if ((errstr = UTL_ILLIOPT("c=R=W=m=p=e=s=o=y=k=r=x=a=b=u=t=i=g=f=1=2=3=4=5=ld=vnh?",
							  errbuf))) {	
		printf("*** %s\n", errstr);
		return(1);
	}

	/* more help requested ? */
	if (UTL_TSTOPT("h")) {						
		usage(1);
		return(1);
	}

	/* normal help requested ? */
	if (UTL_TSTOPT("?")) {
		usage(0);
		return(1);
	}

	/* option specific help */
	if ((str=UTL_TSTOPT("R=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_RdModeInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("W=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_WrModeInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("m=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_CntModeInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("p=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_PreClrCondInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("e=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_PreClrCondInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("s=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_StoreCondInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("o=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_CompIrqInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("y=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_CybwIrqInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("k=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_EnbIrqInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("r=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_EnbIrqInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("x=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_EnbIrqInfo);
		return(1);
	}

	if ((str=UTL_TSTOPT("t=")) && *str == '?') {
		PrintInfo(NULL, TRUE, G_TimerStartInfo);
		return(1);
	}

	if (((str=UTL_TSTOPT("a=")) && *str == '?') ||
		((str=UTL_TSTOPT("b=")) && *str == '?') ||
		((str=UTL_TSTOPT("u=")) && *str == '?') ||
		((str=UTL_TSTOPT("d=")) && *str == '?') ||
		((str=UTL_TSTOPT("i=")) && *str == '?') ||
		((str=UTL_TSTOPT("g=")) && *str == '?') || 
		((str=UTL_TSTOPT("f=")) && *str == '?') ||
		((str=UTL_TSTOPT("1=")) && *str == '?') ||
		((str=UTL_TSTOPT("2=")) && *str == '?') ||
		((str=UTL_TSTOPT("3=")) && *str == '?') ||
		((str=UTL_TSTOPT("4=")) && *str == '?') ||
		((str=UTL_TSTOPT("5=")) && *str == '?')) {
		printf("(no detailed help available)\n");
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	for (device=NULL, n=1; n<argc; n++)
		if (*argv[n] != '-') {
			device = argv[n];
			break;
		}

	if (!device) {
		usage(0);
		return(1);
	}

	chan      = ((str = UTL_TSTOPT("c=")) ? atoi(str) : -1);
	readMode  = ((str = UTL_TSTOPT("R=")) ? atoi(str) : -1);
	writeMode = ((str = UTL_TSTOPT("W=")) ? atoi(str) : -1);
	cntMode   = ((str = UTL_TSTOPT("m=")) ? atoi(str) : -1);
	preCond   = ((str = UTL_TSTOPT("p=")) ? atoi(str) : -1);
	clrCond   = ((str = UTL_TSTOPT("e=")) ? atoi(str) : -1);
	storeCond = ((str = UTL_TSTOPT("s=")) ? atoi(str) : -1);
	compIrq   = ((str = UTL_TSTOPT("o=")) ? atoi(str) : -1);
	cybwIrq   = ((str = UTL_TSTOPT("y=")) ? atoi(str) : -1);
	timerStart= ((str = UTL_TSTOPT("t=")) ? atoi(str) : -1);
	ReadySig= ((str = UTL_TSTOPT("1=")) ? atoi(str) : 0);
	CompSig = ((str = UTL_TSTOPT("2=")) ? atoi(str) : 0);
	CybwSig = ((str = UTL_TSTOPT("3=")) ? atoi(str) : 0);
	LbreakSig=((str = UTL_TSTOPT("4=")) ? atoi(str) : 0);
	Xin2Sig = ((str = UTL_TSTOPT("5=")) ? atoi(str) : 0);
	lbreakIrq = ((str = UTL_TSTOPT("k=")) ? atoi(str) : -1);
	xin2Irq   = ((str = UTL_TSTOPT("r=")) ? atoi(str) : -1);
	chanIrq   = ((str = UTL_TSTOPT("x=")) ? atoi(str) : -1);
	loopDelay = ((str = UTL_TSTOPT("d=")) ? atoi(str) : 200);
	valPreload= ((str = UTL_TSTOPT("u=")) ? UTL_Atox(str) : 0);
	loopMode  = (UTL_TSTOPT("l") ? 1 : 0);
	dontRead  = (UTL_TSTOPT("n") ? 1 : 0);
	G_Verbose = (UTL_TSTOPT("v") ? 1 : 0);

	if ((str = UTL_TSTOPT("a="))) {
		valCompA = UTL_Atox(str);
		setCompA = TRUE;
	}

	if ((str = UTL_TSTOPT("b="))) {
		valCompB = UTL_Atox(str);
		setCompB = TRUE;
	}

	if ((str = UTL_TSTOPT("u="))) {
		valPreload = UTL_Atox(str);
		setPreload = TRUE;
	}

	if ((str = UTL_TSTOPT("i="))) {
		valOutMode = UTL_Atox(str);
		setOutMode = TRUE;
	}

	if ((str = UTL_TSTOPT("g="))) {
		valOutSet = UTL_Atox(str);
		setOutSet = TRUE;
	}

	if ((str = UTL_TSTOPT("f="))) {
		valSelftest = UTL_Atox(str);
		setSelftest = TRUE;
	}

    /*--------------------+
    |  install signals    |
    +--------------------*/
	/* install signal handler */
	if( (error = UOS_SigInit(SigHandler)) ){	
		printf("*** can't UOS_SigInit: %s\n",UOS_ErrString(error));
		return(1);
	}

	/* install signals */
	if ((ReadySig  && (error = UOS_SigInstall(ReadySig ))) ||
		(CompSig   && (error = UOS_SigInstall(CompSig  ))) ||
		(CybwSig   && (error = UOS_SigInstall(CybwSig  ))) ||
		(LbreakSig && (error = UOS_SigInstall(LbreakSig))) ||
		(Xin2Sig   && (error = UOS_SigInstall(Xin2Sig)))) {
		printf("*** can't UOS_SigInstall: %s\n",UOS_ErrString(error));
		goto abort;
	}

	/* mask all signals */
	UOS_SigMask();

	/*--------------------+
    |  open path          |
    |  set current chan   |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

    /*--------------------+
    |  config driver      |
    +--------------------*/
	if (G_Verbose)
		printf("Driver Configuration\n");

	if (SetGetStat(path, (chan != -1), &chan,
				   M_MK_CH_CURRENT, "M_MK_CH_CURRENT",
				   "current channel", G_ChanInfo))
		goto abort;

	if (SetGetStat(path, (readMode != -1), &readMode,
				   M72_READ_MODE, "M72_READ_MODE",
				   "mode for read  calls", G_RdModeInfo))
		goto abort;

	if (SetGetStat(path, (writeMode != -1), &writeMode,
				   M72_WRITE_MODE, "M72_WRITE_MODE",
				   "mode for write calls", G_WrModeInfo))
		goto abort;


    /*--------------------+
    |  config counter     |
    +--------------------*/
	if (G_Verbose)
		printf("\nCounter Configuration\n");

	if (SetGetStat(path, setCompA, &valCompA,
				   M72_VAL_COMPA, "M72_VAL_COMPA",
				   "comparator A value", NULL))
		goto abort;

	if (SetGetStat(path, setCompB, &valCompB,
				   M72_VAL_COMPB, "M72_VAL_COMPB",
				   "comparator B value", NULL))
		goto abort;

	if (SetGetStat(path, (cntMode != -1), &cntMode,
				   M72_CNT_MODE, "M72_CNT_MODE",
				   "counter mode", G_CntModeInfo))
		goto abort;

	if (SetGetStat(path, (preCond != -1), &preCond,
				   M72_CNT_PRELOAD, "M72_CNT_PRELOAD",
				   "counter preload condition", G_PreClrCondInfo))
		goto abort;

	if (SetGetStat(path, (clrCond != -1), &clrCond,
				   M72_CNT_CLEAR, "M72_CNT_CLEAR",
				   "counter clear   condition", G_PreClrCondInfo))
		goto abort;

	if (SetGetStat(path, (storeCond != -1), &storeCond,
				   M72_CNT_STORE, "M72_CNT_STORE",
				   "counter store   condition", G_StoreCondInfo))
		goto abort;

	if (SetGetStat(path, (compIrq != -1), &compIrq,
				   M72_COMP_IRQ, "M72_COMP_IRQ",
				   "comparator   irq condition", G_CompIrqInfo))
		goto abort;

	if (SetGetStat(path, (cybwIrq != -1), &cybwIrq,
				   M72_CYBW_IRQ, "M72_CYBW_IRQ",
				   "carry/borrow irq condition", G_CybwIrqInfo))
		goto abort;

	if (SetGetStat(path, (lbreakIrq != -1), &lbreakIrq,
				   M72_LBREAK_IRQ, "M72_LBREAK_IRQ",
				   "line-break irq enable", G_EnbIrqInfo))
		goto abort;

	if (SetGetStat(path, (xin2Irq != -1), &xin2Irq,
				   M72_XIN2_IRQ, "M72_XIN2_IRQ",
				   "xIN2 edge  irq enable", G_EnbIrqInfo))
		goto abort;

	if (SetGetStat(path, (timerStart != -1), &timerStart,
				   M72_TIMER_START, "M72_TIMER_START",
				   "timer start condition", G_TimerStartInfo))
		goto abort;

	if (SetGetStat(path, setOutMode, &valOutMode,
				   M72_OUT_MODE, "M72_OUT_MODE",
				   "output signal mode", NULL))
		goto abort;

	if (SetGetStat(path, setOutSet, &valOutSet,
				   M72_OUT_SET, "M72_OUT_SET",
				   "output signal setting", NULL))
		goto abort;

	if (SetGetStat(path, setSelftest, &valSelftest,
				   M72_SELFTEST, "M72_SELFTEST",
				   "selftest configuration", NULL))
		goto abort;

	if (SetGetStat(path,(chanIrq != -1), &chanIrq,				
				   M72_ENB_IRQ, "M72_ENB_IRQ",
				   "irq enable per channel", G_EnbIrqInfo))
		goto abort;

    /*--------------------+
    |  activate signals   |
    +--------------------*/
	if (G_Verbose && (ReadySig | CompSig | CybwSig | LbreakSig | Xin2Sig))
		printf("\nDriver Signals\n");

	if (SetGetStat(path, ReadySig, (int32*)&ReadySig,
				   M72_SIGSET_READY, "M72_SIGSET_READY",
				   "ready   signal", NULL))
		goto abort;

	if (SetGetStat(path, (int32)CompSig, (int32*)&CompSig,
				   M72_SIGSET_COMP, "M72_SIGSET_COMP",
				   "comparator   signal", NULL))
		goto abort;

	if (SetGetStat(path, (int32)CybwSig, (int32*)&CybwSig,
				   M72_SIGSET_CYBW, "M72_SIGSET_CYBW",
				   "carry/borrow signal", NULL))
		goto abort;

	if (SetGetStat(path, (int32)LbreakSig, (int32*)&LbreakSig,
				   M72_SIGSET_LBREAK, "M72_SIGSET_LBREAK",
				   "line-break   signal", NULL))
		goto abort;

	if (SetGetStat(path, (int32)Xin2Sig, (int32*)&Xin2Sig,
				   M72_SIGSET_XIN2, "M72_SIGSET_XIN2",
				   "XIN2         signal", NULL))
		goto abort;

    /*--------------------+
    |  preload counter    |
    +--------------------*/
	if (setPreload) {
		printf("\n");

		if (G_Verbose)
			printf("\nCounter Preload\n");

		printf("preload counter value=0x%08lx\n", valPreload);

		if ((ret = M_write(path, valPreload)) < 0) {
			PrintError("write");
			goto abort;
		}
	}

    /*--------------------+
    |  enable interrupt   |
    +--------------------*/
	if (M_setstat(path, M_MK_IRQ_ENABLE, 1) < 0) {
		PrintError("setstat M_MK_IRQ_ENABLE");
		goto abort;
	}
	intEn = TRUE;

    /*--------------------+
    |  read loop ...      |
    +--------------------*/
	printf("\n");

	if (G_Verbose)
		printf("\nMeasurement (%s)\n",loopMode ? "Loop":"Single");

	printf("(Press any key for exit)\n");

	/* unmask all signals */
	UOS_SigUnMask();

	do {
		/* signal handling */
		if (G_Signal) {
			printf("\n");

			if (G_Signal == ReadySig) 
				printf(">>> measurement ready\n");
			else if (G_Signal == CompSig) 
				printf(">>> comparator match\n");
			else if (G_Signal == CybwSig) 
				printf(">>> carry/borrow occurred\n");
			else if (G_Signal == LbreakSig) 
				printf(">>> line-break detected\n");
			else if (G_Signal == Xin2Sig)					
				printf(">>> XIN2 detected\n");
			else 
				printf(">>> signal=%ld received\n",G_Signal);
			
			G_Signal = 0;
		}
		
		/* FREQ mode: start measurement */
		if (cntMode == M72_MODE_FREQ) {
			printf("\nstart freq measurement\n");

			if (M_setstat(path, M72_FREQ_START, 0) < 0) {
				PrintError("setstat M72_FREQ_START");
				goto abort;
			}
		}

		/* read/print counter */
		if (dontRead == FALSE) {
			printf("read counter ");
			fflush(stdout);

			if ((ret = M_read(path,&valCount)) < 0) {
				printf("\n");
				PrintError("read");
				break;
			}

			switch(cntMode) {
				case M72_MODE_FREQ:
					printf("value=0x%08lx freq=%ld Hz",
						   valCount, valCount * 100);
					break;
				case M72_MODE_PULSEH:
				case M72_MODE_PULSEL:
				case M72_MODE_PERIOD:
					printf("value=0x%08lx time=%f sec",
						   valCount, (float)valCount / 2500000);
					break;
				default:
					printf("value=0x%08lx", valCount);
			}

			fflush(stdout);
		}

		/* key check, delay */
		if (UOS_KeyPressed() != -1)
			break;

		if (loopMode && loopDelay)
			UOS_Delay(loopDelay);

		/* linefeed or line clear */
		if (dontRead == FALSE) {
			switch(cntMode) {
				case M72_MODE_FREQ:
				case M72_MODE_PULSEH:
				case M72_MODE_PULSEL:
				case M72_MODE_PERIOD:
					printf("\n");
					break;
				default:
					for (n=0; n<29; n++)
						putchar('\b');
			}
		}

		/* PULSEx/PERIOD mode: start next measurement (if required) */
		if (cntMode == M72_MODE_PULSEH ||
			 cntMode == M72_MODE_PULSEL ||
			 cntMode == M72_MODE_PERIOD) {
			
			if ( clrCond == M72_CLEAR_NOW) {
				printf("\nstart measurement (with counter clear)\n");
	
				if (M_setstat(path, M72_CNT_CLEAR, M72_CLEAR_NOW) < 0) {
					PrintError("setstat M72_CNT_CLEAR");
					goto abort;
				}
			}

			if ( preCond == M72_PRELOAD_NOW) {
				printf("\nstart measurement (with counter preload)\n");
	
				if (M_setstat(path, M72_CNT_PRELOAD, M72_PRELOAD_NOW) < 0) {
					PrintError("setstat M72_CNT_PRELOAD");
					goto abort;
				}
			}


		}

	} while(loopMode);

	printf("\n");

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:

    /* disable interrupt when enabled */
	if (intEn) {
		if (M_setstat(path, M_MK_IRQ_ENABLE, 0) < 0) {
			PrintError("setstat M_MK_IRQ_ENABLE");
		}

	}
    /* deactivate signals */
	if (ReadySig && M_setstat(path, M72_SIGCLR_READY, 0) < 0)
		PrintError("setstat M72_SIGCLR_READY");

	if (CompSig && M_setstat(path, M72_SIGCLR_COMP, 0) < 0)
		PrintError("setstat M72_SIGCLR_COMP");

	if (CybwSig && M_setstat(path, M72_SIGCLR_CYBW, 0) < 0) 
		PrintError("setstat M72_SIGCLR_CYBW");

	if (LbreakSig && M_setstat(path, M72_SIGCLR_LBREAK, 0) < 0) 
		PrintError("setstat M72_SIGCLR_LBREAK");

	if (Xin2Sig && M_setstat(path, M72_SIGCLR_XIN2, 0) < 0) 
		PrintError("setstat M72_SIGCLR_XIN2");

	UOS_Delay(500);

    /* close */
	if (M_close(path) < 0)
		PrintError("close");

	/* terminate signal handling */
	UOS_SigExit();						

	return(0);
}

/********************************* PrintError ********************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/

static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}

/********************************* PrintInfo ********************************
 *
 *  Description: Print multiple info lines
 *			   
 *---------------------------------------------------------------------------
 *  Input......: prefix  prefix string or NULL (added to each line)
 *               prval	 enable value (index) is print
 *               info	 NULL terminated info string array or NULL
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/

static void PrintInfo(char *prefix, u_int32 prval, char **info)
{
	u_int32 n=0;

	if (info) {
		while(*info) {
			if (**info)	{
				if (prefix)
					printf("%s",prefix);

				if (prval)
					printf("%ld = ",n);

				printf("%s\n",*info);
			}

			info++;
			n++;
		}
	}
}

/********************************* SetGetStat *******************************
 *
 *  Description: Set/Get device status and print info/error messages.
 *			   
 *               Depending on 'setstat', the function calls M_getstat and
 *               returns the read value in 'valueP' or calls M_setstat and
 *               writes the value given in 'valueP'. 
 *			   
 *---------------------------------------------------------------------------
 *  Input......: path    device path
 *               setstat 0=getstat, 1=setstat
 *               code	 status code 
 *               valueP	 ptr to value (if setstat)
 *               name	 status code name
 *               desc	 status code description
 *               info	 NULL terminated info string array or NULL
 *  Output.....: valueP	 ptr to read value (if getstat)
 *               return  success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 SetGetStat(
	MDIS_PATH path,	
	int32 setstat,
	int32 *valueP,
	int32 code,
	char *name,
	char *desc,
	char **info
)
{
	int32 maxInfo=0;
	int32 error=0;

	/* count known info strings */
	if (info) {
		while(info[maxInfo])
			maxInfo++;
	}

	/* call setstat or getstat */
	if (setstat) {
		printf("set %-28s: ", desc);

		if (info && IN_RANGE(*valueP, 0, maxInfo))
			printf("%s\n", info[*valueP]);
		else
			printf("%-4ld (0x%lx)\n", *valueP, *valueP);

		if ((M_setstat(path, code, *valueP)) < 0) {
			error = UOS_ErrnoGet();
			printf("*** can't setstat %s: %s\n", name, M_errstring(error));
			return(error);
		}
	}
	else {
		if ((M_getstat(path, code, valueP)) < 0) {
			error = UOS_ErrnoGet();
			printf("*** can't getstat %s: %s\n", name, M_errstring(error));
			return(error);
		}

		if (G_Verbose) {
			printf("get %-28s: ", desc);

			if (info && IN_RANGE(*valueP, 0, maxInfo))
				printf("%s\n", info[*valueP]);
			else
				printf("%ld (0x%lx)\n", *valueP, *valueP);
		}
	}

	return(error);
}







 
