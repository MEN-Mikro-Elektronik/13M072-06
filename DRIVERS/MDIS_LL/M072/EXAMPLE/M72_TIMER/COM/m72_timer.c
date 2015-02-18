/****************************************************************************
 ************                                                    ************
 ************                   M72_TIMER                        ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *        $Date: 2010/04/20 15:04:37 $
 *    $Revision: 1.8 $
 *
 *  Description: M72 example for timer mode
 *                      
 *               Configuration:
 *               - timer mode
 *               - signal on comparator match (1sec: 0x2625a0)
 *               - clear counter with comparator match
 *               - start timer initially with counter clear
 *               - read mode: latch before read
 *               - NOTE: timer direction must be UP
 *               
 *               Measurement:
 *               - read counter of channel 0
 *               - wait for signals
 *               - calculate timestamps
 *                      
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_timer.c,v $
 * Revision 1.8  2010/04/20 15:04:37  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.7  2006/09/27 18:55:58  ts
 * changed return type of main() to int due to Compiler warnings
 *
 * Revision 1.6  2004/08/30 12:19:46  dpfeuffer
 * minor modifications for MDIS4/2004 conformity
 *
 * Revision 1.5  1999/08/06 15:07:22  Franke
 * cosmetics
 *
 * Revision 1.4  1999/08/06 10:30:28  Schoberl
 * added headers stdlib.h, string.h
 *
 * Revision 1.3  1999/08/06 09:19:29  Schoberl
 * cosmetics
 * functions static
 * main directly called in all systems now
 * added:
 * - setstat for IRQ enable per channel
 * - deinstall comparator signal if installed
 * removed:
 * - printf in SigHandler
 *
 * Revision 1.2  1998/10/30 15:22:11  Schmidt
 * m72_timer : unreferenced local variable 't1' and 't2' removed
 *
 * Revision 1.1  1998/10/23 16:31:47  see
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/m72_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define LOOPDELAY	250		/* loop delay [ms] */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int32 G_lastTime, G_deltaTime, G_signalCode;
static u_int32 G_compMatch=FALSE, G_unkwnSig=FALSE;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void __MAPILIB SigHandler( u_int32 sigCode );
static void PrintError(char *info);

/********************************* SigHandler *******************************
 *
 *  Description: Signal handler
 *			   
 *---------------------------------------------------------------------------
 *  Input......: sigCode	signal code received
 *  Output.....: -
 *  Globals....: G_lastTime, G_deltaTime, G_compMatch, G_unkwnSig, G_signalCode
 ****************************************************************************/

static void __MAPILIB SigHandler( u_int32 sigCode )
{
	u_int32 currTime;
	currTime = UOS_MsecTimerGet();

	switch (sigCode)
	{
	case UOS_SIG_USR1:
		G_compMatch = TRUE;
	    G_deltaTime = currTime - G_lastTime;
		break;
	default:
		G_signalCode = sigCode;
		G_unkwnSig = TRUE;
	}

	G_lastTime = currTime;
}

/********************************* main *************************************
 *
 *  Description: MAIN entry
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		command line arguments/counter
 *  Output.....: return    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc,char *argv[])
{
	MDIS_PATH path=0;
    int32 error;
	int32 chan;
	u_int32 count, usrSig = FALSE;
	char *device;

	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m72_timer <device> <chan>\n");
		printf("Function: M72 example for timer mode\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("    chan         channel number (0..n)\n");
		printf("\n");
		return(1);
	}

	device = argv[1];
	chan = atoi(argv[2]);

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

    /*--------------------+
    |  install signals    |
    +--------------------*/
	/* install signal handler */
	if( (error = UOS_SigInit(SigHandler)) ) {	
		printf("*** can't UOS_SigInit: %s\n",UOS_ErrString(error));
		return(1);
	}

	/* install signal */
	if( (error = UOS_SigInstall(UOS_SIG_USR1)) ) {
		printf("*** can't UOS_SigInstall: %s\n",UOS_ErrString(error));
		goto abort;
	}

	/*--------------------+
    |  config             |
    +--------------------*/
	/* channel number */
	if ((M_setstat(path, M_MK_CH_CURRENT, chan)) < 0) {
		PrintError("setstat M_MK_CH_CURRENT");
		goto abort;
	}

	/* read mode: latch before read */
	if ((M_setstat(path, M72_READ_MODE, M72_READ_NOW)) < 0) {
		PrintError("setstat M72_READ_MODE");
		goto abort;
	}

	/* comparator A value (0x2625a0) */
	if ((M_setstat(path, M72_VAL_COMPA, 0x2625a0)) < 0) {
		PrintError("setstat M72_VAL_COMPA");
		goto abort;
	}

	/* counter mode: timer */
	if ((M_setstat(path, M72_CNT_MODE, M72_MODE_TIMER)) < 0) {
		PrintError("setstat M72_CNT_MODE");
		goto abort;
	}

	/* counter clear at comparator match */
	if ((M_setstat(path, M72_CNT_CLEAR, M72_CLEAR_COMP)) < 0) {
		PrintError("setstat M72_CNT_CLEAR");
		goto abort;
	}

	/* comparator irq if equal */
	if ((M_setstat(path, M72_COMP_IRQ, M72_COMP_EQUAL)) < 0) {
		PrintError("setstat M72_COMP_IRQ");
		goto abort;
	}

	/* install comparator signal */
	if ((M_setstat(path, M72_SIGSET_COMP, UOS_SIG_USR1)) < 0) {
		PrintError("setstat M72_SIGSET_COMP");
		goto abort;
	}
	usrSig = TRUE;

	/* timer start: now */
	if ((M_setstat(path, M72_TIMER_START, M72_TIMER_NOW)) < 0) {
		PrintError("setstat M72_TIMER_START");
		goto abort;
	}

	/* irq enable for channel */
	if ((M_setstat(path, M72_ENB_IRQ, TRUE)) < 0) {
		PrintError("setstat M72_ENB_IRQ");
		goto abort;
	}

	/* enable interrupt */
	if ((M_setstat(path, M_MK_IRQ_ENABLE, 1)) < 0) {
		PrintError("setstat M_MK_IRQ_ENABLE");
		goto abort;
	}

	/*--------------------+
    |  measurement loop   |
    +--------------------*/
	printf("Configuration:\n");
	printf("- timer mode\n");
	printf("- signal on comparator match (1sec: 0x2625a0)\n");
	printf("- clear counter with comparator match\n");
	printf("- start timer initially with counter clear\n");
	printf("- read mode: latch before read\n");
	printf("- NOTE: timer direction must be UP\n");
	printf("\n");
	printf("Measurement:\n");
	printf("- read counter of channel %ld\n", chan);
	printf("- wait for signals\n");
	printf("- calculate timestamps\n");
	printf("(Press any key for exit)\n");
	printf("\n");

	/* start timer initially */
	printf("start timer (clear counter)\n");

	if (M_setstat(path, M72_CNT_CLEAR, M72_CLEAR_NOW) < 0) {
		PrintError("setstat M72_CNT_CLEAR");
		goto abort;
	}

	/* initial timestamp */
	G_lastTime = UOS_MsecTimerGet();

	do {
		if (G_compMatch) {
			printf(">>> comparator match after %ld msec\n", G_deltaTime);
			G_compMatch = FALSE;
		}
		if (G_unkwnSig) {
			printf(">>> signal=%ld received\n",G_signalCode);
			G_unkwnSig = FALSE;
		}

		/* read counter */
		if (M_read(path, (int32*)&count) < 0) {
			PrintError("read");
			break;
		}

		/* print result */
		printf("counter=0x%08lx\n", count);

		/* wait some time */
		UOS_Delay(LOOPDELAY);

	} while(UOS_KeyPressed() == -1);	/* while no key pressed */

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:
	/* deinstall comparator signal if installed */
	if (usrSig) {
		if ((M_setstat(path, M72_SIGCLR_COMP, 0)) < 0) {
			PrintError("setstat M72_SIGCLR_COMP");
		}
	}
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





