/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m72_drv.c
 *      Project: M72 module driver (MDIS V4.x)
 *
 *       Author: see
 *        $Date: 2010/04/20 15:10:39 $
 *    $Revision: 1.6 $
 *
 *  Description: Low-level driver for M72 M-Modules
 *
 *               The M72 M-Module is 4-channel motion counter with various
 *               counter/timer modes and interrupt capabilities.
 *               4 output lines allow signaling of counter events (IRQs)
 *               or may be used as simple binary outputs.
 *
 *               The driver handles the M72 counters as 4 channels:
 *
 *                   Channel 0 = Counter A 			(in/out)
 *                   Channel 1 = Counter B  		(in/out)
 *                   Channel 2 = Counter C 			(in/out)
 *                   Channel 3 = Counter D  		(in/out)
 *
 *               Writing to a channel loads the counter, reading from a
 *               channel reads the counter, depending on the read/write mode
 *               used. Block I/O is not supported.
 *
 *               The mode for write and read access can be configured.
 *
 *               Counter store/preload/clear conditions can be configured.
 *
 *               Interrupts can be configured/enabled for several events.
 *
 *               Interrupts can send definable user signals.
 *
 *     Required: OSS, DESC, PLD, ID, DBG libraries
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_drv_pretrig.c,v $
 * Revision 1.6  2010/04/20 15:10:39  amorbach
 * R.1: Porting to MDIS5
 *   2: MDVE test failed
 * M.1: Changed according to MDIS Porting Guide 0.8
 *   2: M72_PldIdent, M72_PldData renamed to __M72_PldIdent, __M72_PldData to enable variant specific substitution
 *
 * Revision 1.5  2008/02/26 17:23:21  ts
 * driver now based on completely reworked FPGA content that fixes several bugs
 * TimerA simultaneously latched/cleared to enable continous period measurement
 * Interrupt enabled with SetStat EN_PRETRIG only
 *
 * Revision 1.4  2007/07/31 17:32:14  ts
 * works with direct CLK-counting between 2 edges (see m72_pretrig.c)
 * cosmetics
 *
 * Revision 1.3  2007/05/24 18:31:30  ts
 * Bugfix: dont return Error when M_MK_IRQ_ENABLE is called
 *
 * Revision 1.2  2006/09/26 18:48:39  ts
 * use PLD file Version depending on Variant
 *
 * Revision 1.1  2006/09/26 18:44:12  ts
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/microwire.h>  /* ID PROM functions              */
#include <MEN/pld_load.h>   /* PLD loader functions           */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */
#include "m72_pld.h"		/* PLD ident/data prototypes      */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			4			/* number of device channels */
#define USE_IRQ				TRUE		/* interrupt required  */
#define ADDRSPACE_COUNT		1			/* number of required address spaces */
#define ADDRSPACE_SIZE		256			/* size of address space */
#define MOD_ID_MAGIC		0x5346      /* ID PROM magic word */
#define MOD_ID_SIZE			128			/* ID PROM size [bytes] */
#define MOD_ID				72			/* ID PROM module ID */
#define SIG_COUNT			5			/* number of signals per channel */

/* The Channels */
#define PRETR_TIMER_A 0
#define PRETR_TIMER_B 1
#define PRETR_TIMER_C 2
#define PRETR_TIMER_D 3

#define TIMER_PRELOAD_MAX	100000000		/* this gives 40s max. period */

/* debug settings */
#define DBG_MYLEVEL			llHdl->dbgLevel
#define DBH					llHdl->dbgHdl

/* register offsets (i=channel) */
#define COUNT_CTRL_REG(i)	(((i)<<5)+0x00)		/* (w) */
#define COMPA_LOW_REG(i)	(((i)<<5)+0x02)		/* (w) */
#define COMPB_LOW_REG(i)	(((i)<<5)+0x04)		/* (w) */
#define PRELOAD_LOW_REG(i)	(((i)<<5)+0x06)		/* (w) */
#define IRQ_CTRL_REG(i)		(((i)<<5)+0x08)		/* (w) */
#define COMPA_HIGH_REG(i)	(((i)<<5)+0x0a)		/* (w) */
#define COMPB_HIGH_REG(i)	(((i)<<5)+0x0c)		/* (w) */
#define PRELOAD_HIGH_REG(i)	(((i)<<5)+0x0e)		/* (w) */
#define COUNT_LOW_REG(i)	(((i)<<5)+0x10)		/* (r) */
#define COUNT_HIGH_REG(i)	(((i)<<5)+0x12)		/* (r) */
#define IRQ_STATE_REG1		0x80				/* (r/w) */
#define IRQ_STATE_REG2		0x82				/* (r/w) */
#define OUT_CTRL1_REG		0x84				/* (w) */
#define OUT_CTRL2_REG		0x86				/* (w) */
#define OUT_CONFIG_REG		0x88				/* (r/w) */
#define PLD_IF_REG			0xfe				/* (r/w) */
#define SELFTEST_REG		0x8a				/* (w) */
#ifdef _BIG_ENDIAN_
#endif
/* COUNT_CTRL_REG */
#define CLEAR_MASK		0x0003
#define PRELOAD_MASK	0x000c
#define STORE_MASK		0x0030
#define TIMEBASE		0x0040
#define TIMER			0x0080
#define MODE_MASK		0x0f00

/* IRQ_CTRL_REG */
#define LBREAK_ENB		0x0001
#define XIN2_ENB		0x0002
#define CYBW_MASK		0x000c
#define COMP_MASK		0x0070
#define ENB_MASK		0x0080

/* IRQ_STATE_REG (i=channel) */
#define READY_PEND(i)	0x1<<((i)<<3)
#define COMP_PEND(i)	0x2<<((i)<<3)
#define CYBW_PEND(i)	0x4<<((i)<<3)
#define LBREAK_PEND(i)	0x8<<((i)<<3)
#define XIN2_PEND(i)	0x10<<((i)<<3)

/* PLD_IF_REG: PLD bit locations */
#define PSDAT			0
#define PSCLK			1
#define PSCONF			3
#define PSSTAT			1
#define PSDONE			2

/* PLD_IF_REG: non-PLD bit state */
#define PLD_IF_MASK		0x00

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* low-level handle */
typedef struct {
	/* general */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* misc */
    u_int32         irqCount;       /* interrupt counter */
    u_int32         idCheck;		/* id check enabled */
	MCRW_HANDLE		*mcrwHdl;
	/* shadow registers */
    u_int16         regCountCtrl[CH_NUMBER];
    u_int16         regIrqCtrl[CH_NUMBER];
    u_int16         regSelftest;
    u_int16         regOutCtrl1;
    u_int16         regOutCtrl2;
	u_int8			regIntStatChan[CH_NUMBER];	/* int.statereg of chan A..D */
	/* read/write mode */
    u_int32         readAvail[CH_NUMBER];	 /* value available (latched) 	*/
    u_int32         readMode[CH_NUMBER];	 /* read mode 					*/
    u_int32         readTimeout[CH_NUMBER];	 /* read timeout 				*/
    u_int32         writeMode[CH_NUMBER];	 /* write mode 					*/
    OSS_SEM_HANDLE  *readSemHdl[CH_NUMBER];  /* ready semaphore (read) 		*/
	/* counter config */
    u_int32         cntMode[CH_NUMBER];		/* counter mode 				*/
    u_int32         cntPreload[CH_NUMBER];	/* counter preload condition 	*/
    u_int32         cntClear[CH_NUMBER];	/* counter clear condition 		*/
    u_int32         cntStore[CH_NUMBER];	/* counter store condition 		*/
	/* irq config */
	u_int32			enbIrq[CH_NUMBER];		/* irq enable per channel 		*/
    u_int32         compIrq[CH_NUMBER];		/* Comparator irq condition 	*/
    u_int32         cybwIrq[CH_NUMBER];		/* Carry/Borrow irq condition 	*/
    u_int32         xin2Irq[CH_NUMBER];	    /* xIN2 Edge  irq enable 		*/
    u_int32         lbreakIrq[CH_NUMBER];	/* Line-Break irq enable 		*/
	/* preload/compare values */
    u_int32         valPreload[CH_NUMBER];	/* preload value 				*/
    u_int32         valCompA[CH_NUMBER];	/* comparator A value 			*/
    u_int32         valCompB[CH_NUMBER];	/* comparator B value 			*/
	/* timer config */
    u_int32         timerStart[CH_NUMBER];	/* timer start condition 		*/
	u_int32 		cntPretrig;				/* clk-count to pretrig. Timer	*/
	u_int32 		enPretrig;				/* effective pretrigger status	*/

	/* signals */
    OSS_SIG_HANDLE  *sigHdl[CH_NUMBER][SIG_COUNT];  /* signal handles */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jumptable  */
#include <MEN/m72_drv.h>   /* M72 driver header file */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 M72_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
					   MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
					   OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 M72_Exit(LL_HANDLE **llHdlP );
static int32 M72_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 M72_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 M72_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 M72_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64);
static int32 M72_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 M72_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 M72_Irq(LL_HANDLE *llHdl );
static int32 M72_Info(int32     infoType, ... );


static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static void CounterStore(LL_HANDLE *llHdl, int32 ch, int32 cond);
static void CounterClear(LL_HANDLE *llHdl, int32 ch, int32 cond);
static void CounterLoad(LL_HANDLE *llHdl, int32 ch, int32 cond);
static void TimerABCsetup( LL_HANDLE *llHdl);

/**************************** M72_GetEntry *********************************
 *
 *  Description:  Initialize driver's jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    extern void LL_GetEntry( LL_ENTRY* drvP )
#else
# ifdef MAC_BYTESWAP
    extern void M72_PRE_SW_GetEntry( LL_ENTRY* drvP )
# else
    extern void M72_PRE_GetEntry( LL_ENTRY* drvP )
# endif
#endif
{
    drvP->init        = M72_Init;
    drvP->exit        = M72_Exit;
    drvP->read        = M72_Read;
    drvP->write       = M72_Write;
    drvP->blockRead   = M72_BlockRead;
    drvP->blockWrite  = M72_BlockWrite;
    drvP->setStat     = M72_SetStat;
    drvP->getStat     = M72_GetStat;
    drvP->irq         = M72_Irq;
    drvP->info        = M72_Info;
}

/******************************** M72_Init ***********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 *
 *                The function initially loads the module's PLD, if
 *                this is not explicitly disabled by PLD_LOAD=0.
 *
 *                Then the function clears all counters and initializes
 *                all channels with the definitions made in the
 *                descriptor.
 *                The counters are configured as follows:
 *                - clear the counter
 *                - write comparator A value
 *                - write comparator B value
 *                - write comparator preload value
 *                - set the clear, load, store (latch) conditions
 *                - set the timer start and counter mode condition
 *                - set the interrupt conditions
 *                then the output settings and output mode is set.
 *                The Self-test Register is set to zero.
 *
 *                The following descriptor keys are used:
 *
 *                Descriptor key         Default          Range
 *                ---------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC       OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL            OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK               1                0..1
 *                PLD_LOAD               1                0..1
 *                OUT_MODE               0                0..max
 *                OUT_SET                0                0..0xf
 *                CHANNEL_n/CNT_MODE     0                0..7,9,10  (1)
 *                CHANNEL_n/CNT_PRELOAD  0                0..3
 *                CHANNEL_n/CNT_CLEAR    0                0..3
 *                CHANNEL_n/CNT_STORE    0                0..2
 *                CHANNEL_n/ENB_IRQ      0                0..1
 *                CHANNEL_n/COMP_IRQ     0                0..5
 *                CHANNEL_n/CYBW_IRQ     0                0..3
 *                CHANNEL_n/LBREAK_IRQ   0                0..1
 *                CHANNEL_n/XIN2_IRQ     0                0..1
 *                CHANNEL_n/VAL_PRELOAD  0                0..max
 *                CHANNEL_n/VAL_COMPA    0                0..max
 *                CHANNEL_n/VAL_COMPB    0                0..max
 *                CHANNEL_n/READ_MODE    2                0..2
 *                CHANNEL_n/READ_TIMEOUT 0xffffffff       0..0xffffffff ms
 *                CHANNEL_n/WRITE_MODE   2                0, 2    (2)
 *                CHANNEL_n/TIMER_START  0                0..1
 *
 *                (1) value 8 is not valid.
 *                (2) only values 0 and 2 are used for write mode.
 *
 *
 *                PLD_LOAD defines if the PLD is loaded at M72_Init.
 *                With PLD_LOAD disabled, ID_CHECK is implicitly disabled.
 *                (This is for test purposes and should always be set to 1.)
 *
 *                OUT_MODE defines the output signal mode.
 *                (see SetStat: M72_OUT_MODE)
 *
 *                OUT_SET defines the output signal setting.
 *                (see SetStat: M72_OUT_SET)
 *
 *                CNT_MODE defines the counter mode of channel n.
 *                (see M72_SetStat: M72_CNT_MODE)
 *                    NOTE: Value 8 is not a valid counter mode!
 *
 *                CNT_PRELOAD defines counter preload condition of channel n.
 *                (see M72_SetStat: M72_CNT_PRELOAD)
 *
 *                CNT_CLEAR defines the counter clear condition of channel n.
 *                (see M72_SetStat: M72_CNT_CLEAR)
 *
 *                CNT_STORE defines the counter store condition of channel n.
 *                (see M72_SetStat: M72_CNT_STORE)
 *
 *                ENB_IRQ defines the interrupt facility of channel n.
 *                (see M72_SetStat: M72_ENB_IRQ)
 *
 *                COMP_IRQ defines the Comparator interrupt condition of
 *                channel n.
 *                (see M72_SetStat: M72_COMP_IRQ)
 *
 *                CYBW_IRQ defines the Carry/Borrow interrupt condition of
 *                channel n.
 *                (see M72_SetStat: M72_CYBW_IRQ)
 *
 *                LBREAK_IRQ enables/disables the Line-Break interrupt of
 *                channel n.
 *                (see M72_SetStat: M72_LBREAK_IRQ)
 *
 *                XIN2_IRQ enables/disables the xIN2 Edge interrupt of
 *                channel n.
 *                (see M72_SetStat: M72_XIN2_IRQ)
 *
 *                VAL_PRELOAD defines the counter preload value of channel n.
 *                (see M72_Write)
 *
 *                VAL_COMPA/B defines the comparator A/B value of channel n.
 *                (see SetStat: M72_VAL_COMPA/B)
 *
 *                READ_MODE defines the mode of driver read calls of channel n.
 *                (see SetStat: M72_READ_MODE)
 *
 *                READ_TIMEOUT defines a timeout in milliseconds for read calls
 *                of channel n, when waiting for a Ready interrupt.
 *                (see SetStat: M72_READ_TIMEOUT)
 *
 *                WRITE_MODE define the mode of write calls of channel n.
 *                (see SetStat: M72_WRITE_MODE)
 *
 *                TIMER_START defines the timer start condition of channel n.
 *
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         hardware access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *  Output.....:  llHdlP     pointer to low-level driver handle
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize, value, loadPld, outMode, outSet, n;
	u_int16 irq_statex;
    int32 error;
	u_int16 modIdMagic;
	u_int16 modId;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	/* alloc */
    if ((*llHdlP = llHdl = (LL_HANDLE*)
		 OSS_MemGet(osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma		  = *ma;

    /*------------------------------+
    |  create semaphores            |
    +------------------------------*/
	for (n=0; n<CH_NUMBER; n++) {
		if ((error = OSS_SemCreate(llHdl->osHdl, OSS_SEM_BIN, 0,
								   &llHdl->readSemHdl[n])))
			return( Cleanup(llHdl,error) );
	}

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	DESC_DbgLevelSet(llHdl->descHdl, value);	/* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&llHdl->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    DBGWRT_1((DBH, "LL - M72_Init\n"));

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE,
								&llHdl->idCheck, "ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /* LOAD_PLD */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE,
								&loadPld, "PLD_LOAD")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	if (loadPld == FALSE)
		llHdl->idCheck = FALSE;

    /* OUT_MODE */
    if ((error = DESC_GetUInt32(llHdl->descHdl, 0x0,
								&outMode, "OUT_MODE")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /* OUT_SET */
    if ((error = DESC_GetUInt32(llHdl->descHdl, 0x0,
								&outSet, "OUT_SET")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	if (outSet > 0xf)
		return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

	/* channel 0..3 params */
	for (n=0; n<CH_NUMBER; n++) {
		/* CNT_MODE */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_MODE_NO,
									&llHdl->cntMode[n],
									"CHANNEL_%d/CNT_MODE", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if ((llHdl->cntMode[n] > 10) || (llHdl->cntMode[n] == 8))
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* CNT_PRELOAD */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_PRELOAD_NO,
									&llHdl->cntPreload[n],
									"CHANNEL_%d/CNT_PRELOAD", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->cntPreload[n] > 3)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* CNT_CLEAR */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_CLEAR_NO,
									&llHdl->cntClear[n],
									"CHANNEL_%d/CNT_CLEAR", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->cntClear[n] > 3)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* CNT_STORE */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_STORE_NO,
									&llHdl->cntStore[n],
									"CHANNEL_%d/CNT_STORE", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->cntStore[n] > 2)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

		/* ENB_IRQ */
		if ((error = DESC_GetUInt32(llHdl->descHdl, FALSE,
									&llHdl->enbIrq[n],
									"CHANNEL_%d/ENB_IRQ", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->enbIrq[n] > 1)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

		/* COMP_IRQ */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_COMP_NO,
									&llHdl->compIrq[n],
									"CHANNEL_%d/COMP_IRQ", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->compIrq[n] > 5)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* CYBW_IRQ */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_CYBW_NO,
									&llHdl->cybwIrq[n],
									"CHANNEL_%d/CYBW_IRQ", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->cybwIrq[n] > 3)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* LBREAK_IRQ */
		if ((error = DESC_GetUInt32(llHdl->descHdl, FALSE,
									&llHdl->lbreakIrq[n],
									"CHANNEL_%d/LBREAK_IRQ", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->lbreakIrq[n] > 1)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* XIN2_IRQ */
		if ((error = DESC_GetUInt32(llHdl->descHdl, FALSE,
									&llHdl->xin2Irq[n],
									"CHANNEL_%d/XIN2_IRQ", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->xin2Irq[n] > 1)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* VAL_PRELOAD */
		if ((error = DESC_GetUInt32(llHdl->descHdl, 0x00000000,
									&llHdl->valPreload[n],
									"CHANNEL_%d/VAL_PRELOAD", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

        /* VAL_COMPA */
		if ((error = DESC_GetUInt32(llHdl->descHdl, 0x00000000,
									&llHdl->valCompA[n],
									"CHANNEL_%d/VAL_COMPA", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

        /* VAL_COMPB */
		if ((error = DESC_GetUInt32(llHdl->descHdl, 0x00000000,
									&llHdl->valCompB[n],
									"CHANNEL_%d/VAL_COMPB", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

        /* READ_MODE */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_READ_NOW,
									&llHdl->readMode[n],
									"CHANNEL_%d/READ_MODE", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->readMode[n] > 2)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* READ_TIMEOUT */
		if ((error = DESC_GetUInt32(llHdl->descHdl, 0xffffffff,
									&llHdl->readTimeout[n],
									"CHANNEL_%d/READ_TIMEOUT", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		/* WRITE_MODE */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_WRITE_NOW,
									&llHdl->writeMode[n],
									"CHANNEL_%d/WRITE_MODE", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if ((llHdl->writeMode[n] > 2) || (llHdl->writeMode[n] == 1))
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );

        /* TIMER_START */
		if ((error = DESC_GetUInt32(llHdl->descHdl, M72_TIMER_IN2,
									&llHdl->timerStart[n],
									"CHANNEL_%d/TIMER_START", n)) &&
			error != ERR_DESC_KEY_NOTFOUND)
			return( Cleanup(llHdl,error) );

		if (llHdl->timerStart[n] > 1)
			return( Cleanup(llHdl,ERR_LL_ILL_PARAM) );
	}

    /*---------------------------+
    |  MICROWIRE Library Handle  |
    +---------------------------*/
	{
		MCRW_DESC_PORT   descMcrw;

		/* clear the structure */
		OSS_MemFill( llHdl->osHdl, sizeof(descMcrw), (char*)&descMcrw, 0 );

		/* bus speed */
		descMcrw.busClock   = 10; /* OSS_MikroDelay */

		/* address length */
		descMcrw.addrLength   = 6; /* for 93C46 */

		/* set FLAGS */
		descMcrw.flagsDataIn   =
		  ( MCRW_DESC_PORT_FLAG_SIZE_16 | MCRW_DESC_PORT_FLAG_READABLE_REG | MCRW_DESC_PORT_FLAG_POLARITY_HIGH );

		descMcrw.flagsDataOut  =
		  ( MCRW_DESC_PORT_FLAG_SIZE_16 | MCRW_DESC_PORT_FLAG_POLARITY_HIGH );

		descMcrw.flagsClockOut =
		  ( MCRW_DESC_PORT_FLAG_SIZE_16 | MCRW_DESC_PORT_FLAG_POLARITY_HIGH );

		descMcrw.flagsCsOut =
		  ( MCRW_DESC_PORT_FLAG_SIZE_16 | MCRW_DESC_PORT_FLAG_POLARITY_HIGH );

		descMcrw.flagsOut   = MCRW_DESC_PORT_FLAG_OUT_IN_ONE_REG;

		/* set addr and mask */
		descMcrw.addrDataIn   = (char*)llHdl->ma + PLD_IF_REG;
		descMcrw.maskDataIn   = 0x0001; /*EEDAT*/

		descMcrw.addrDataOut  = (char*)llHdl->ma + PLD_IF_REG;
		descMcrw.maskDataOut  = 0x0001; /*EEDAT*/
		descMcrw.notReadBackDefaultsDataOut = 0xFFFE;
		descMcrw.notReadBackMaskDataOut     = 0xFFFE;

		descMcrw.addrClockOut = (char*)llHdl->ma + PLD_IF_REG;
		descMcrw.maskClockOut = 0x0002; /*EECLK*/
		descMcrw.notReadBackDefaultsClockOut = 0xFFFD;
		descMcrw.notReadBackMaskClockOut     = 0xFFFD;

		descMcrw.addrCsOut	 = (char*)llHdl->ma + PLD_IF_REG;
		descMcrw.maskCsOut	 = 0x0004; /*EECS*/
		descMcrw.notReadBackDefaultsCsOut = 0xFFFB;
		descMcrw.notReadBackMaskCsOut     = 0xFFFB;

	    error = MCRW_PORT_Init( &descMcrw,
			                   	  llHdl->osHdl,
   	        		              (void**)&llHdl->mcrwHdl );
   	    if( error || llHdl->mcrwHdl == NULL )
   	    {
             DBGWRT_ERR( ( DBH, " *** M72_Init: MCRW_PORT_Init() error=%d\n",
                           error ));
             error = ERR_ID;
             return( Cleanup(llHdl,error) );
   	    }/*if*/
	}/*MCRW INIT*/

    /*------------------------------+
    |  check module ID              |
    +------------------------------*/
    if( llHdl->idCheck)
    {
    	/* read MAGIC and module type */
        error = llHdl->mcrwHdl->ReadEeprom( (void*)llHdl->mcrwHdl, 0, &modIdMagic,
                                               sizeof(modIdMagic) );
        if( error )
        {
             DBGWRT_ERR( ( DBH, " *** M72_Init: ReadEeprom() error=%d\n",
                          error ));
             error = ERR_ID_NOTFOUND;
             return( Cleanup(llHdl,error) );
        }/*if*/
        error = llHdl->mcrwHdl->ReadEeprom( (void*)llHdl->mcrwHdl, 2, &modId,
                                               sizeof(modId) );
        if( error )
        {
             DBGWRT_ERR( ( DBH, " *** M72_Init: ReadEeprom() error=%d\n",
                          error ));
             error = ERR_ID_NOTFOUND;
             return( Cleanup(llHdl,error) );
        }/*if*/

		if (modIdMagic != MOD_ID_MAGIC) {
			DBGWRT_ERR((DBH," *** M72_Init: illegal magic=0x%04x\n",modIdMagic));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
		if (modId != MOD_ID) {
			DBGWRT_ERR((DBH," *** M72_Init: illegal id=%d\n",modId));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
	}

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	llHdl->idFuncTbl.idCall[1].identCall = __M72_PldIdent;
	/* library's ident functions */
	llHdl->idFuncTbl.idCall[2].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[3].identCall = OSS_Ident;
	llHdl->idFuncTbl.idCall[4].identCall = llHdl->mcrwHdl->Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[5].identCall = NULL;

	/*------------------------------+
    |  load PLD                     |
    +------------------------------*/
	if (loadPld) {
	    MACCESS   maPld;
		u_int32 size;
		u_int8  *dataP = (u_int8*)__M72_PldData; 		/* point to binary data */

		MACCESS_CLONE(llHdl->ma, maPld, PLD_IF_REG);/* create access handle */

		/* read+skip size */
		size  = (u_int32)(*dataP++) << 24;
		size |= (u_int32)(*dataP++) << 16;
		size |= (u_int32)(*dataP++) <<  8;
		size |= (u_int32)(*dataP++);

		/* load data */
		DBGWRT_2((DBH," load PLD: size=%d\n",size));

		if((error=PLD_FLEX10K_LoadDirect(&maPld, dataP, size,
										 PLD_FIRSTBLOCK+PLD_LASTBLOCK,
										 llHdl->osHdl,
										 (void (*)(void*, u_int32))OSS_Delay,
										 PLD_IF_MASK,
										 PSDAT,PSCLK, PSCONF, PSSTAT, PSDONE)))
			return( Cleanup(llHdl, ERR_PLD+error) );
	}

    /*------------------------------+
    |  init hardware                |
    |  (registers and shadow array) |
    +------------------------------*/
	llHdl->irqCount = 0;

	/* pretrigger disabled on startup */
	llHdl->enPretrig = 0;

	/* clear pending irqs in irq state register 1*/
	if( (irq_statex = MREAD_D16(llHdl->ma, IRQ_STATE_REG1)) )
		MWRITE_D16(llHdl->ma, IRQ_STATE_REG1, irq_statex);

	/* clear pending irqs in irq state register 2*/
	if( (irq_statex = MREAD_D16(llHdl->ma, IRQ_STATE_REG2)) )
		MWRITE_D16(llHdl->ma, IRQ_STATE_REG2, irq_statex);

	/* channel 0..3 config */
	for (n=0; n<CH_NUMBER; n++) {
		/* clear shadow register for Interrupt Status Register */
		llHdl->regIntStatChan[n] = 0;

		/* clear counter */
		CounterClear(llHdl, n, M72_CLEAR_NOW);

		/* comparator A value */
		MWRITE_D16(llHdl->ma, COMPA_LOW_REG(n),
				   (u_int16)(llHdl->valCompA[n] & 0xffff));
		MWRITE_D16(llHdl->ma, COMPA_HIGH_REG(n),
				   (u_int16)(llHdl->valCompA[n] >> 16));

		/* comparator B value */
		MWRITE_D16(llHdl->ma, COMPB_LOW_REG(n),
				   (u_int16)(llHdl->valCompB[n] & 0xffff));
		MWRITE_D16(llHdl->ma, COMPB_HIGH_REG(n),
				   (u_int16)(llHdl->valCompB[n] >> 16));

		/* counter preload value */
		MWRITE_D16(llHdl->ma, PRELOAD_LOW_REG(n),
				   (u_int16)(llHdl->valPreload[n] & 0xffff));
		MWRITE_D16(llHdl->ma, PRELOAD_HIGH_REG(n),
				   (u_int16)(llHdl->valPreload[n] >> 16));

		/* counter config */
		CounterClear(llHdl, n, llHdl->cntClear[n]);
		CounterLoad(llHdl, n, llHdl->cntPreload[n]);
		CounterStore(llHdl, n, llHdl->cntStore[n]);

		llHdl->regCountCtrl[n] |= llHdl->timerStart[n] << 7;
		llHdl->regCountCtrl[n] |= llHdl->cntMode[n]	   << 8;
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(n), llHdl->regCountCtrl[n]);

		/* comparator/irq config */
		llHdl->regIrqCtrl[n] = (u_int16)    (
			(llHdl->lbreakIrq[n]	<< 0)	|
			(llHdl->xin2Irq[n]		<< 1)	|
			(llHdl->cybwIrq[n]		<< 2)	|
			(llHdl->compIrq[n]		<< 4)   |
			(llHdl->enbIrq[n]       << 7)   );

		MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(n), llHdl->regIrqCtrl[n]);
	}

	/* output setting */
	MWRITE_D16(llHdl->ma, OUT_CONFIG_REG, (u_int16)(outSet));

	/* output mode */
	llHdl->regOutCtrl1 = (u_int16)(outMode & 0xffff);
	llHdl->regOutCtrl2 = (u_int16)(outMode >> 16);
	MWRITE_D16(llHdl->ma, OUT_CTRL1_REG, llHdl->regOutCtrl1);
	MWRITE_D16(llHdl->ma, OUT_CTRL2_REG, llHdl->regOutCtrl2);

	/* selftest config */
	llHdl->regSelftest = 0x0000;
	MWRITE_D16(llHdl->ma, SELFTEST_REG, llHdl->regSelftest);

	return(ERR_SUCCESS);
}

/****************************** M72_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *                The function deinitializes all channels by disabling all
 *                counters (MODE=0) and all interrupt conditions.
 *
 *                All pending interrupts are cleared.
 *                The cleanup function is called.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP  	pointer to low-level driver handle
 *  Output.....:  return    success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
	u_int32 n;
	u_int16 irq_statex;
	int32 error = 0;

	DBGWRT_1((DBH, "LL - M72_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	/* for channels 0..3 */
	for (n=0; n<CH_NUMBER; n++) {
		/* counter config */
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(n), 0x0000);

		/* comparator/irq config */
		MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(n), 0x0000);

		/* clear shadow register for Interrupt Status Register */
		llHdl->regIntStatChan[n] = 0;
	}

	/* output config */
	MWRITE_D16(llHdl->ma, OUT_CTRL1_REG, 0x0000);
	MWRITE_D16(llHdl->ma, OUT_CTRL2_REG, 0x0000);
	MWRITE_D16(llHdl->ma, OUT_CONFIG_REG, 0x0000);

	/* selftest config */
	MWRITE_D16(llHdl->ma, SELFTEST_REG, 0x0000);

	/* clear pending irqs in irq state register 1*/
	if ((irq_statex = MREAD_D16(llHdl->ma, IRQ_STATE_REG1)))
		MWRITE_D16(llHdl->ma, IRQ_STATE_REG1, irq_statex);

	/* clear pending irqs in irq state register 2*/
	if( (irq_statex = MREAD_D16(llHdl->ma, IRQ_STATE_REG2)) )
		MWRITE_D16(llHdl->ma, IRQ_STATE_REG2, irq_statex);

    /*------------------------------+
    |  clean up memory              |
    +------------------------------*/
	error = Cleanup(llHdl,error);

	return(error);
}

/****************************** M72_Read *************************************
 *
 *  Description:  Read a value from the device
 *
 *                Depending on the configured read mode, the following actions
 *                may be executed BEFORE the counter latch is read:
 *
 *                - M72_READ_LATCH
 *                  nothing
 *                - M72_READ_WAIT (1)
 *                  wait for Ready interrupt or timeout
 *                - M72_READ_NOW
 *                  force counter latch
 *
 *                (1) NOTE: With read mode M72_READ_WAIT, the IRQEN bit in
 *                          the Interrupt Control Register should always be
 *                          enabled.
 *
 *                Then the function reads the latched counter of the current
 *                channel as a 32-bit value.
 *
 *                See also: Counter latch condition (M72_CNT_STORE setstat)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *value
)
{
	u_int32 low_word, high_word;
	int32 error;
	OSS_IRQ_STATE oldState;

	DBGWRT_1((DBH, "LL - M72_Read: ch=%d:\n",ch));

	/*----------------------------+
	|  wait for Ready irq         |
	+----------------------------*/
	if (llHdl->readMode[ch] == M72_READ_WAIT) {
		DBGWRT_2((DBH, " wait for ready irq ..\n"));
		if ((error = OSS_SemWait(llHdl->osHdl, llHdl->readSemHdl[ch],
								 llHdl->readTimeout[ch])))
			return(error);
	}

	/*----------------------------+
	|  force counter latch        |
	+----------------------------*/
	if (llHdl->readMode[ch] == M72_READ_NOW)
		CounterStore(llHdl, ch, M72_STORE_NOW);

	/*----------------------------+
	|  read counter latch         |
	+----------------------------*/
	oldState = OSS_IrqMaskR(llHdl->osHdl, llHdl->irqHdl);

	low_word  = MREAD_D16(llHdl->ma, COUNT_LOW_REG(ch));
	high_word = MREAD_D16(llHdl->ma, COUNT_HIGH_REG(ch));
	*value = low_word | (high_word << 16);

	OSS_IrqRestore(llHdl->osHdl, llHdl->irqHdl, oldState);

	DBGWRT_2((DBH, " read latch=0x%08x\n",*value));

	return(ERR_SUCCESS);
}

/****************************** M72_Write ************************************
 *
 *  Description:  Write a value to the device
 *
 *                The function loads the counter preload register of the
 *                current channel with a 32-bit value.
 *
 *                Depending on the configured write mode, the following actions
 *                may be executed AFTER the preload register is written:
 *
 *                - M72_WRITE_PRELOAD
 *                  nothing
 *                - M72_WRITE_NOW
 *                  force counter load
 *
 *                See also: Counter preload condition (M72_CNT_PRELOAD setstat)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *                value    value to write
 *  Output.....:  return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{

    DBGWRT_1((DBH, "LL - M72_Write: ch=%d\n",ch));

	/*----------------------------+
	|  write counter preload      |
	+----------------------------*/
    DBGWRT_2((DBH, " write preload=0x%08x\n",value));

	MWRITE_D16(llHdl->ma, PRELOAD_LOW_REG(ch),  (u_int16)(value & 0xffff));
	MWRITE_D16(llHdl->ma, PRELOAD_HIGH_REG(ch), (u_int16)(value >> 16));

	/*----------------------------+
	|  force counter load         |
	+----------------------------*/
	if (llHdl->writeMode[ch] == M72_WRITE_NOW)
		CounterLoad(llHdl, ch, M72_PRELOAD_NOW);

	return(ERR_SUCCESS);
}

/****************************** M72_SetStat **********************************
 *
 *  Description:  Set the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see oss.h
 *                M_MK_IRQ_ENABLE      interrupt enable           0..1
 *                M_LL_IRQ_COUNT       interrupt counter          0..max
 *                M_LL_CH_DIR          direction of curr. chan.   M_CH_INOUT
 *                -------------------  -------------------------  ----------
 *                M72_CNT_MODE         counter mode               0..7,9,10
 *                M72_CNT_PRELOAD      counter preload condition  0..3
 *                M72_CNT_CLEAR        counter clear   condition  0..3
 *                M72_CNT_STORE        counter store   condition  0..2
 *                M72_ENB_IRQ          interrupt enable           0..1
 *                M72_COMP_IRQ    	   Comparator   irq condition 0..5
 *                M72_CYBW_IRQ    	   Carry/Borrow irq condition 0..3
 *                M72_LBREAK_IRQ  	   Line-Break   irq enable    0..1
 *                M72_XIN2_IRQ   	   xIN2 Edge    irq enable    0..1
 *                M72_INT_STATUS       irq status bits            0..0x1f
 *                M72_VAL_COMPA        comparator A value         0..max
 *                M72_VAL_COMPB        comparator B value         0..max
 *                M72_READ_MODE        mode for read calls        0..2
 *                M72_READ_TIMEOUT     timeout for read calls     0..0xffffffff
 *                M72_WRITE_MODE       mode for write calls       0, 2    (1)
 *                M72_TIMER_START      timer start condition      0..1
 *                M72_FREQ_START       start frequency measurem.  -
 *                M72_SIGSET_READY     install Ready        sig.  1..max
 *                M72_SIGSET_COMP      install Comparator   sig.  1..max
 *                M72_SIGSET_CYBW      install Carry/Borrow sig.  1..max
 *                M72_SIGSET_LBREAK    install Line-Break   sig.  1..max
 *                M72_SIGSET_XIN2      install xIN2 Edge    sig.  1..max
 *                M72_SIGCLR_READY     deinst. Ready        sig.  -
 *                M72_SIGCLR_COMP      deinst. Comparator   sig.  -
 *                M72_SIGCLR_CYBW      deinst. Carry/Borrow sig.  -
 *                M72_SIGCLR_LBREAK    deinst. Line-Break   sig.  -
 *                M72_SIGCLR_XIN2      deinst. xIN2 Edge    sig.  -
 *                M72_OUT_MODE         output signal mode           (2)
 *                M72_OUT_SET          output signal setting      0..0xf
 *                M72_SELFTEST         selftest register      no statements (3)
 *                -------------------  -------------------------  ----------
 *                Note: for values see also m72_drv.h
 *
 *                (1) only values 0 and 2 are used for write mode.
 *                (2) see description below and m72_drv.h
 *                (3) the M72 M-Module supports factory self tests. These tests
 *                    are controlled through the Self-test Register.
 *                    !! A USER SHOULD NEVER PROGRAM THE SELF-TEST REGISTER !!
 *
 *                M72_CNT_MODE defines the counter mode of the current channel:
 *
 *                    M72_MODE_NO       0x00    no count (halted)
 *                    M72_MODE_SINGLE   0x01    single count
 *                    M72_MODE_1XQUAD   0x02    1x quadrature count
 *                    M72_MODE_2XQUAD   0x03    2x quadrature count
 *                    M72_MODE_4XQUAD   0x04    4x quadrature count
 *                    M72_MODE_FREQ     0x05    frequency measurement
 *                    M72_MODE_PULSEH   0x06    pulse width high
 *                    M72_MODE_PULSEL   0x07    pulse width low
 *                    M72_MODE_PERIOD   0x09    period measurement
 *                    M72_MODE_TIMER    0x0a    timer mode
 *
 *
 *                M72_CNT_PRELOAD, defines the counter preload condition,
 *                of the current channel, i.e. when the preload value is
 *                moved into the counter:
 *
 *                    M72_PRELOAD_NO    0x00    no condition (disabled)
 *                    M72_PRELOAD_IN2   0x01    at xIN2 rising edge
 *                    M72_PRELOAD_NOW   0x02    now (immediately)
 *                    M72_PRELOAD_COMP  0x03    at comparator match
 *
 *                    NOTE: M72_PRELOAD_NOW preloads the counter immediately.
 *                          Afterwards the previous counter preload condition
 *                          is restored.
 *
 *
 *                M72_CNT_CLEAR defines the counter clear condition
 *                of the current channel, i.e. when the counter value is to
 *                be cleared:
 *
 *                    M72_CLEAR_NO      0x00    no condition (disabled)
 *                    M72_CLEAR_IN2     0x01    at xIN2 rising edge
 *                    M72_CLEAR_NOW     0x02    now (immediately)
 *                    M72_CLEAR_COMP    0x03    at comparator match
 *
 *                    NOTE: M72_CLEAR_NOW clears the counter immediately.
 *                          Afterwards the previous counter clear condition
 *                          is restored.
 *
 *
 *                M72_CNT_STORE defines the counter store condition
 *                of the current channel, i.e. when the counter value is to
 *                be latched:
 *
 *                    M72_STORE_NO      0x00    no condition (disabled)
 *                    M72_STORE_NOW     0x01    now (immediately)
 *                    M72_STORE_IN2     0x02    at xIN2 rising edge
 *
 *                    NOTE: M72_STORE_NOW latches the counter immediately.
 *                          Afterwards the previous counter store condition
 *                          is restored.
 *
 *
 *                M72_ENB_IRQ enables or disables the ability of the current
 *                channel to generate an interrupt (sets IRQEN for the current
 *                channel in the channel's Interrupt Control Register):
 *
 *                    0 = disable
 *                    1 = enable
 *
 *                    NOTE: An IRQ is only triggered if IRQEN is set to 1.
 *                          Disabling also clears the Interrupt Status Shadow
 *                          Register for the channel.
 *
 *
 *                M72_COMP_IRQ defines the comparator interrupt condition
 *                of the current channel:
 *
 *                    M72_COMP_NO       0x00    no condition (disabled)
 *                    M72_COMP_LESS     0x01    if counter < COMPA
 *                    M72_COMP_GREATER  0x02    if counter > COMPA
 *                    M72_COMP_EQUAL    0x03    if counter = COMPA
 *                    M72_COMP_INRANGE  0x04    if COMPA < counter < COMPB
 *                    M72_COMP_OUTRANGE 0x05    if counter < COMPA or
 *                                                 counter > COMPB
 *
 *                M72_CYBW_IRQ defines the carry/borrow interrupt condition
 *                of the current channel:
 *
 *                    M72_CYBW_NO       0x00    no condition (disabled)
 *                    M72_CYBW_CY       0x01    if carry
 *                    M72_CYBW_BW       0x02    if borrow
 *                    M72_CYBW_CYBW     0x03    if carry or borrow
 *
 *
 *                M72_LBREAK_IRQ enables/disables the line-break interrupt
 *                of the current channel:
 *
 *                    0 = disable
 *                    1 = enable
 *
 *                    NOTE: When a line break is detected, the interrupt
 *                          service routine disables the Line-Break interrupt.
 *                          (see M72_Irq)
 *
 *
 *                M72_XIN2_IRQ enables/disables the xIN2 rising edge interrupt
 *                of the current channel:
 *
 *                    0 = disable
 *                    1 = enable
 *
 *
 *                M72_INT_STATUS clears the interrupt status flags.
 *                Writing 1 to the respective bit will reset the bit.
 *                - If IRQEN is set in the Interrupt Control Register, the
 *                  bits are cleared in the Interrupt Status Shadow Register.
 *                - If IRQEN is not set in the Interrupt Control Register,
 *                  the bits are cleared in the Interrupt Status Register on
 *                  hardware.
 *
 *                    NOTE: When interrupt status register is polled, events
 *                          might not be detected.
 *
 *
 *                M72_VAL_COMPA/B loads the comparator A/B with a 32-bit value.
 *
 *
 *                M72_READ_MODE defines the mode for read calls of the current
 *                channel, i.e. additional actions to be executed BEFORE the
 *                counter latch is read:
 *
 *                    M72_READ_LATCH    0x00    read latched value
 *                    M72_READ_WAIT		0x01	wait for Ready irq
 *                    M72_READ_NOW		0x02	force counter latch
 *
 *                    NOTE: For mode M72_READ_WAIT the IRQ must be enabled
 *                          for the respective channel (IRQEN must be set in
 *							the Interrupt Control Register).
 *
 *                M72_READ_TIMEOUT defines a timeout in milliseconds for read
 *                calls of the current channel, when waiting for a Ready
 *                interrupt.
 *
 *
 *                M72_WRITE_MODE defines the mode for write calls of the curr.
 *                channel, i.e. additional actions which are executed AFTER the
 *                counter preload is written:
 *
 *                    M72_WRITE_PRELOAD 0x00    load preload register
 *                    M72_WRITE_NOW		0x02	force counter load
 *
 *
 *                M72_TIMER_START defines the timer start condition of the
 *                current channel (for the timer mode):
 *
 *                    M72_TIMER_IN2     0x00    at xIN2 rising edge
 *                    M72_TIMER_NOW     0x01    now (immediately)
 *
 *
 *                M72_FREQ_START starts the frequency measurement of the
 *                current channel (for the frequ. meas. mode) by clearing
 *                the counter and starting the gate timer (10ms).
 *                ERR_LL_ILL_PARAM is returned for all other counter modes.
 *
 *
 *                M72_SIGSET_READY  install Ready              signal
 *                M72_SIGSET_COMP   install Comparator match   signal
 *                M72_SIGSET_CYBW   install Carry/Borrow       signal
 *                M72_SIGSET_LBREAK install Line-Break         signal
 *                M72_SIGSET_XIN2   install xIN2 Edge          signal
 *                    M72_SIGSET_xxx installs (activates) a user signal for
 *                    the current channel.
 *                    The installed user signal is sent if the corresponding
 *                    interrupt condition (see M72_xxx_IRQ) occurs.
 *                    (See also M72_Irq)
 *
 *
 *                M72_SIGCLR_READY  deinstall Ready            signal
 *                M72_SIGCLR_COMP   deinstall Comparator match signal
 *                M72_SIGCLR_CYBW   deinstall Carry/Borrow     signal
 *                M72_SIGCLR_LBREAK deinstall Line-Break       signal
 *                M72_SIGCLR_XIN2   deinstall xIN2 Edge        signal
 *                    M72_SIGCLR_xxx deinstalls (deactivates) a user signal
 *                    for the current channel.
 *
 *
 *                M72_OUT_MODE defines the output signal mode, i.e. the
 *                action of signals Out_1..4, which are influenced by
 *                Carry/Borrow and Compare interrupt events of any channel
 *                (see also hardware manual).
 *                Use the M72_OUTCFG(out,ch,mode) macro to create control
 *                word flags. The flags can be ORed:
 *
 *                    M72_OUTCFG(1,0,mode)  Out_1 action on channel 0
 *                    M72_OUTCFG(1,1,mode)  Out_1 action on channel 1
 *                    ...                   ...
 *                    M72_OUTCFG(4,3,mode)  Out_4 action on channel 3
 *
 *                Use the following values for 'mode':
 *
 *                    M72_OUT_MODE_NO    0x00   no action on interrupt
 *                    M72_OUT_MODE_HIGH  0x01   high signal on interrupt
 *                    M72_OUT_MODE_LOW   0x02   low signal on interrupt
 *                    M72_OUT_MODE_TOGLE 0x03   toggle signal on interrupt
 *
 *
 *                M72_OUT_SET defines the output signal setting flags:
 *
 *                    M72_OUT_SET_CTRL1  0x01   Control_1 signal
 *                    M72_OUT_SET_CTRL2  0x02   Control_2 signal
 *                    M72_OUT_SET_CTRL3  0x04   Control_3 signal
 *                    M72_OUT_SET_CTRL4  0x08   Control_4 signal
 *
 *                Setting a flag sets the corresponding Out_x signal to
 *                high, clearing a flag sets the signal to low.
 *                To affect several output lines, the flags can be logically
 *                combined.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64     data or
 *                                  pointer to block data structure (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
	int32 error = ERR_SUCCESS;
    int32       value = (int32)value32_or_64;
    /*INT32_OR_64 valueP = value32_or_64; */

	DBGWRT_1((DBH, "LL - M72_SetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
			/* no special global IRQ support on M72 */


			error = ERR_SUCCESS;
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
			if (value != M_CH_INOUT)
				error = ERR_LL_ILL_DIR;

            break;
        /*--------------------------+
        |   counter mode            |
        +--------------------------*/
        case M72_CNT_MODE :
			if (!IN_RANGE(value,0,10))
				return(ERR_LL_ILL_PARAM);

			if ( value ==8 )
				return(ERR_LL_ILL_PARAM);

			llHdl->cntMode[ch] = value;
			llHdl->regCountCtrl[ch] &= ~MODE_MASK;
			llHdl->regCountCtrl[ch] |= (value << 8);
			MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
            break;
        /*--------------------------+
        |   counter preload cond.   |
        +--------------------------*/
        case M72_CNT_PRELOAD:
			if (!IN_RANGE(value,0,3))
				return(ERR_LL_ILL_PARAM);

			CounterLoad(llHdl, ch, value);
            break;
        /*--------------------------+
        |   counter clear cond.     |
        +--------------------------*/
        case M72_CNT_CLEAR:
			if (!IN_RANGE(value,0,3))
				return(ERR_LL_ILL_PARAM);

			CounterClear(llHdl, ch, value);
            break;
        /*--------------------------+
        |   counter store cond.     |
        +--------------------------*/
        case M72_CNT_STORE:
			if (!IN_RANGE(value,0,2))
				return(ERR_LL_ILL_PARAM);

			CounterStore(llHdl, ch, value);
            break;
        /*--------------------------+
        |   irq enable cond.        |
        +--------------------------*/
        case M72_ENB_IRQ:
			if ( (value == 0) || (value == 1) ) {
				llHdl->enbIrq[ch] = value;
				llHdl->regIrqCtrl[ch] &= ~ENB_MASK;
				llHdl->regIrqCtrl[ch] |= (u_int16)(value << 7);
				MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(ch), llHdl->regIrqCtrl[ch]);

				if (value == 0) {
					llHdl->regIntStatChan[ch] = 0;
				}
            }
			else {
				return(ERR_LL_ILL_PARAM);
			}
			break;

		/*--------------------------+
        |   comparator irq cond.    |
        +--------------------------*/
        case M72_COMP_IRQ:
			if (!IN_RANGE(value,0,5))
				return(ERR_LL_ILL_PARAM);

			llHdl->compIrq[ch] = value;
			llHdl->regIrqCtrl[ch] &= ~COMP_MASK;
			llHdl->regIrqCtrl[ch] |= (u_int16)(value << 4);
			MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(ch), llHdl->regIrqCtrl[ch]);
            break;
        /*--------------------------+
        |   carry/borrow irq cond.  |
        +--------------------------*/
        case M72_CYBW_IRQ:
			if (!IN_RANGE(value,0,3))
				return(ERR_LL_ILL_PARAM);

			llHdl->cybwIrq[ch] = value;
			llHdl->regIrqCtrl[ch] &= ~CYBW_MASK;
			llHdl->regIrqCtrl[ch] |= (u_int16)(value << 2);
			MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(ch), llHdl->regIrqCtrl[ch]);
            break;
        /*--------------------------+
        |   line-break irq enable   |
        +--------------------------*/
        case M72_LBREAK_IRQ:
			if (!IN_RANGE(value,0,1))
				return(ERR_LL_ILL_PARAM);

			llHdl->lbreakIrq[ch] = value;
			llHdl->regIrqCtrl[ch] &= ~LBREAK_ENB;
			llHdl->regIrqCtrl[ch] |= (u_int16)(value << 0);
			MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(ch), llHdl->regIrqCtrl[ch]);
            break;
        /*--------------------------+
        |   xIN2 irq enable         |
        +--------------------------*/
        case M72_XIN2_IRQ:
#if 0 /* do NOT en/disable the IRQ here because in this driver thats done
	   in EN_PRETRIG Setstat! */
			if (!IN_RANGE(value,0,1))
				return(ERR_LL_ILL_PARAM);
			llHdl->xin2Irq[ch] = value;
			llHdl->regIrqCtrl[ch] &= ~XIN2_ENB;
			llHdl->regIrqCtrl[ch] |= (u_int16)(value << 1);
			MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(ch), llHdl->regIrqCtrl[ch]);
#endif
			break;
		/*--------------------------+
        |   interrupt status        |
        +--------------------------*/
        case M72_INT_STATUS:
		{
			OSS_IRQ_STATE oldState;

			if ( (value < 0) || (value > 0x1f) )
				return(ERR_LL_ILL_PARAM);

			if (llHdl->enbIrq[ch]) {	/* access to shadow register */
				oldState = OSS_IrqMaskR(llHdl->osHdl, llHdl->irqHdl);
				llHdl->regIntStatChan[ch] &= (u_int8)(~value);
				OSS_IrqRestore(llHdl->osHdl, llHdl->irqHdl, oldState);
			}
			else {						/* access to int. stat. reg. */
				switch (ch) {
					case 0: MWRITE_D16(llHdl->ma, IRQ_STATE_REG1,
									   (u_int16)value); break;
					case 1: MWRITE_D16(llHdl->ma, IRQ_STATE_REG1,
									   (u_int16)(value<<8)); break;
					case 2: MWRITE_D16(llHdl->ma, IRQ_STATE_REG2,
									   (u_int16)value); break;
					case 3: MWRITE_D16(llHdl->ma, IRQ_STATE_REG2,
									   (u_int16)(value<<8)); break;
				}
			}
			break;
		}
		/*--------------------------+
        |   comparator A value      |
        +--------------------------*/
	    case M72_VAL_COMPA:
			llHdl->valCompA[ch] = value;
			MWRITE_D16(llHdl->ma, COMPA_LOW_REG(ch),
					   (u_int16)(llHdl->valCompA[ch] & 0xffff));
			MWRITE_D16(llHdl->ma, COMPA_HIGH_REG(ch),
					   (u_int16)(llHdl->valCompA[ch] >> 16));
            break;
        /*--------------------------+
        |   comparator B value      |
        +--------------------------*/
        case M72_VAL_COMPB:
			llHdl->valCompB[ch] = value;
			MWRITE_D16(llHdl->ma, COMPB_LOW_REG(ch),
					   (u_int16)(llHdl->valCompB[ch] & 0xffff));
			MWRITE_D16(llHdl->ma, COMPB_HIGH_REG(ch),
					   (u_int16)(llHdl->valCompB[ch] >> 16));
            break;
        /*--------------------------+
        |   read mode               |
        +--------------------------*/
        case M72_READ_MODE:
			if (!IN_RANGE(value,0,2))
				return(ERR_LL_ILL_PARAM);

			llHdl->readMode[ch] = value;
            break;
        /*--------------------------+
        |   read timeout            |
        +--------------------------*/
	    case M72_READ_TIMEOUT:
			llHdl->readTimeout[ch] = value;
            break;


       /*--------------------------+
        |   pretriggering Value    |
        +--------------------------*/
	    case M72_CNT_PRETRIG:
			if (value < 25 ) {
				llHdl->cntPretrig = 25;
			} else {
				llHdl->cntPretrig = value;
			}

			break;

       /*--------------------------+
        | En/disable pretriggering |
        +--------------------------*/
	    case M72_EN_PRETRIG:
			if (!IN_RANGE(value,0,1))
				return(ERR_LL_ILL_PARAM);

			llHdl->enPretrig = value; /* passed ASYNCHRONOUS to IRQ! */

			if(llHdl->enPretrig) {
				DBGWRT_1((DBH, "M72_EN_PRETRIG: Pretrigger ON\n"));
				
				TimerABCsetup(llHdl);
				/* irq enabling for this whole channel A */
				llHdl->enbIrq[0] = TRUE;
				llHdl->regIrqCtrl[0] &= ~ENB_MASK;
				llHdl->regIrqCtrl[0] |= (u_int16)(TRUE << 7);
				MWRITE_D16(llHdl->ma,IRQ_CTRL_REG(0),llHdl->regIrqCtrl[0]);

				llHdl->xin2Irq[0] 	= 1;
				llHdl->regIrqCtrl[0] &= ~XIN2_ENB;
				llHdl->regIrqCtrl[0] |= (u_int16)(1 << 1);
				MWRITE_D16(llHdl->ma,IRQ_CTRL_REG(0),llHdl->regIrqCtrl[0]);

			} else { /* App switched pretrig now OFF */
				DBGWRT_1((DBH, "M72_EN_PRETRIG: Pretrigger OFF\n"));
				/* handled in M72_Irq for synchronous signal shutdown */
			}

			break;

		/*--------------------------+
        |   write mode              |
        +--------------------------*/
        case M72_WRITE_MODE:
			if (value == 0 || value == 2) {
				llHdl->writeMode[ch] = value;
			}
			else {
				return(ERR_LL_ILL_PARAM);
			}

            break;
        /*--------------------------+
        |   timer start condition   |
        +--------------------------*/
        case M72_TIMER_START:
			if (!IN_RANGE(value,0,1))
				return(ERR_LL_ILL_PARAM);

			llHdl->timerStart[ch] = value;
			llHdl->regCountCtrl[ch] &= ~TIMER;
			llHdl->regCountCtrl[ch] |= (u_int16)(value << 7);
			MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
            break;
        /*--------------------------+
        |   start frequency meas.   |
        +--------------------------*/
        case M72_FREQ_START:
			if (llHdl->cntMode[ch] != M72_MODE_FREQ)
				return(ERR_LL_ILL_PARAM);

			MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch),
					   llHdl->regCountCtrl[ch] | TIMEBASE);
			break;
        /*--------------------------+
        |   output signal mode      |
        +--------------------------*/
        case M72_OUT_MODE:
			llHdl->regOutCtrl1 = (u_int16)(value & 0xffff);
			llHdl->regOutCtrl2 = (u_int16)(value >> 16);
			MWRITE_D16(llHdl->ma, OUT_CTRL1_REG, llHdl->regOutCtrl1);
			MWRITE_D16(llHdl->ma, OUT_CTRL2_REG, llHdl->regOutCtrl2);
			break;
		/*-------------------------+
        |   output signal setting  |
        +--------------------------*/
        case M72_OUT_SET:
			if (!IN_RANGE(value,0,0x0f))
				return(ERR_LL_ILL_PARAM);

			MWRITE_D16(llHdl->ma, OUT_CONFIG_REG, (u_int16)(value & 0xf));
			break;
        /*--------------------------+
        |   selftest register config|
        +--------------------------*/
        case M72_SELFTEST:
			llHdl->regSelftest = (u_int16)value;
			MWRITE_D16(llHdl->ma, SELFTEST_REG, llHdl->regSelftest);
			break;

		/*--------------------------+
        |  signal installation      |
        +--------------------------*/
        case M72_SIGSET_READY:
        case M72_SIGSET_COMP:
        case M72_SIGSET_CYBW:
        case M72_SIGSET_LBREAK:
		case M72_SIGSET_XIN2:
		{
			/* get index */
			u_int32 sigNr = code - (M72_SIGSET);

			/* already defined ? */
			if (llHdl->sigHdl[ch][sigNr] != NULL) {
				DBGWRT_ERR((DBH,
							" *** M72_SetStat: signal already installed\n"));
				return(ERR_OSS_SIG_SET);
			}

			/* illegal signal code ? */
			if (value == 0)
				return(ERR_LL_ILL_PARAM);

			/* install signal */
			if ((error = OSS_SigCreate(llHdl->osHdl, value,
									   &llHdl->sigHdl[ch][sigNr])))
				return(error);

            break;
		}
        /*--------------------------+
        |  signal deinstallation    |
        +--------------------------*/
        case M72_SIGCLR_READY:
        case M72_SIGCLR_COMP:
        case M72_SIGCLR_CYBW:
        case M72_SIGCLR_LBREAK:
		case M72_SIGCLR_XIN2:
		{
			/* get index */
			u_int32 sigNr = code - (M72_SIGCLR);

			/* not defined ? */
			if (llHdl->sigHdl[ch][sigNr] == NULL) {
				DBGWRT_ERR((DBH, " *** M72_SetStat: signal not installed\n"));
				return(ERR_OSS_SIG_CLR);
			}

			/* remove signal */
			if ((error = OSS_SigRemove(llHdl->osHdl,
									   &llHdl->sigHdl[ch][sigNr])))
				return(error);

            break;
		}
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** M72_GetStat **********************************
 *
 *  Description:  Get the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see oss.h
 *                M_LL_CH_NUMBER       number of channels         4
 *                M_LL_CH_DIR          direction of curr. chan.   M_CH_INOUT
 *                M_LL_CH_LEN          length of curr. ch. [bits] 32
 *                M_LL_CH_TYP          description of curr. ch.   M_CH_COUNTER
 *                M_LL_IRQ_COUNT       interrupt counter          0..max
 *                M_LL_ID_CHECK        EEPROM is checked          0..1
 *                M_LL_ID_SIZE         EEPROM size [bytes]        128
 *                M_LL_BLK_ID_DATA     EEPROM raw data            -
 *                M_MK_BLK_REV_ID      ident function table ptr   -
 *                -------------------  -------------------------  ----------
 *                M72_CNT_MODE         counter mode               0..7,9,10
 *                M72_CNT_PRELOAD      counter preload condition  0..3
 *                M72_CNT_CLEAR        counter clear   condition  0..3
 *                M72_CNT_STORE        counter store   condition  0..2
 *                M72_ENB_IRQ          irq enable                 0..1
 *                M72_COMP_IRQ    	   Comparator   irq condition 0..5
 *                M72_CYBW_IRQ    	   Carry/Borrow irq condition 0..3
 *                M72_LBREAK_IRQ  	   Line-Break   irq enable    0..1
 *                M72_XIN2_IRQ   	   xIN2 Edge    irq enable    0..1
 *                M72_INT_STATUS       irq status bits            0..0x1f
 *                M72_VAL_COMPA        comparator A value         0..max
 *                M72_VAL_COMPB        comparator B value         0..max
 *                M72_READ_MODE        mode for read calls        0..2
 *                M72_READ_TIMEOUT     timeout for read calls     0..0xffffffff
 *                M72_WRITE_MODE       mode for write calls       0, 2    (1)
 *                M72_TIMER_START      timer start condition      0..1
 *                M72_SIGSET_READY     Ready        signal        0..max
 *                M72_SIGSET_COMP      Comparator   signal        0..max
 *                M72_SIGSET_CYBW      Carry/Borrow signal        0..max
 *                M72_SIGSET_LBREAK    Line-Break   signal        0..max
 *                M72_SIGSET_XIN2      xIN2 Edge    signal        0..max
 *                M72_OUT_MODE         output signal mode           (2)
 *                M72_OUT_SET          output signal setting      0..0xf
 *                M72_SELFTEST         selftest register      no statements (3)
 *                -------------------  -------------------------  ----------
 *                Note: For values see also m72_drv.h.
 *
 *                (1) only values 0 and 2 are used for write mode.
 *                (2) see description below and m72_drv.h
 *                (3) The M72 module supports factory self tests.
 *                    These test are controlled thru the Self-test Register.
 *                    !! A USER SHOULD NEVER PROGRAM THE SELF-TEST REGISTER !!
 *
 *                M72_CNT_MODE returns the counter mode of the current channel.
 *
 *                M72_CNT_PRELOAD, M72_CNT_CLEAR and M72_CNT_STORE return the
 *                counter preload/clear/store condition of the current channel.
 *
 *                M72_ENB_IRQ returns the interrupt enable condition
 *                of the current channel.
 *
 *                M72_COMP_IRQ returns the Comparator interrupt condition
 *                of the current channel.
 *
 *                M72_CYBW_IRQ returns the Carry/Borrow interrupt condition
 *                of the current channel.
 *
 *                M72_LBREAK_IRQ returns if the Line-Break interrupt
 *                of the current channel is enabled/disabled.
 *
 *                M72_XIN2_IRQ returns if the xIN2 rising edge interrupt
 *                of the current channel is enabled/disabled.
 *
 *                M72_INT_STATUS returns the interrupt status flags.
 *                - If IRQEN is set in the Interrupt Control Register, the
 *                  bits are read from the Interrupt Status Shadow Register,
 *                  which is updated (ORed) in the interrupt service routine.
 *                - If IRQEN is not set in the Interrupt Control Register,
 *                  the bits are read from the Interrupt Status Register on
 *                  hardware.
 *
 *                    NOTE: When interrupt status register is polled, events
 *                          might not be detected.
 *
 *                M72_VAL_COMPA/B returns the comparator A/B 32-bit value.
 *
 *                M72_READ_MODE returns the read mode of the current channel.
 *
 *                M72_READ_TIMEOUT returns the timeout in milliseconds for read
 *                calls of the current channel, when waiting for a measurement
 *                ready interrupt.
 *
 *                M72_WRITE_MODE returns the write mode of the current channel.
 *
 *                M72_SIGSET_xxx returns the signal code of an installed
 *                signal for the current channel. Zero is returned if no
 *                signal is installed.
 *
 *                M72_OUT_MODE returns the output signal mode.
 *
 *                M72_OUT_SET returns the current state of the output signals.
 *
 *                (See corresponding codes at M72_SetStat for details)
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64P    pointer to block data structure (M_SG_BLOCK)  (*) 
 *                (*) = for block status codes
 *  Output.....:  value32_or_64P    data pointer or
 *                                  pointer to block data structure (M_SG_BLOCK)  (*) 
 *                return            success (0) or error code
 *                (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64  *value32_or_64P
)
{
    int32       *valueP     = (int32*)value32_or_64P; /* pointer to 32 bit value */
    /* INT32_OR_64 *value64P   = value32_or_64P;         stores 32/64bit pointer */
	M_SG_BLOCK  *blk        = (M_SG_BLOCK*)value32_or_64P;
    int32 dummy;
	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - M72_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_INOUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 32;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_COUNTER;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   ID PROM size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |   ID PROM data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
		{
			if (blk->size < MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);
			/* read MOD_ID_SIZE words */
		        error = llHdl->mcrwHdl->ReadEeprom( (void*)llHdl->mcrwHdl,
													0,
													(u_int16*)blk->data,
													MOD_ID_SIZE );

				if( error )	{
					DBGWRT_ERR( ( DBH,
								  " *** M72_GetStat: ReadEeprom() error=%d\n",
						          error ));
					error = ERR_ID;
		        }/*if*/
			break;
		}
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *valueP = (int32)&llHdl->idFuncTbl;
           break;
        /*--------------------------+
        |   counter mode            |
        +--------------------------*/
        case M72_CNT_MODE :
			*valueP = llHdl->cntMode[ch];
            break;
        /*--------------------------+
        |   counter preload cond.   |
        +--------------------------*/
        case M72_CNT_PRELOAD:
			*valueP = llHdl->cntPreload[ch];
            break;

        /*--------------------------+
        |   TimerB pretrigger Value |
        +--------------------------*/
        case M72_CNT_PRETRIG:
			*valueP = llHdl->cntPretrig;
            break;

       /*--------------------------+
        | pretrigger en/disabled?  |
        +--------------------------*/
	    case M72_EN_PRETRIG:
			*valueP = llHdl->enPretrig;
		break;

        /*--------------------------+
        |   counter clear cond.     |
        +--------------------------*/
        case M72_CNT_CLEAR:
			*valueP = llHdl->cntClear[ch];
            break;
        /*--------------------------+
        |   counter store cond.     |
        +--------------------------*/
        case M72_CNT_STORE:
			*valueP = llHdl->cntStore[ch];
            break;
        /*--------------------------+
        |   irq enable cond.        |
        +--------------------------*/
        case M72_ENB_IRQ:
			*valueP = llHdl->enbIrq[ch];
            break;
        /*--------------------------+
        |   comparator irq cond.    |
        +--------------------------*/
        case M72_COMP_IRQ:
			*valueP = llHdl->compIrq[ch];
            break;
        /*--------------------------+
        |   carry/borrow irq cond.  |
        +--------------------------*/
        case M72_CYBW_IRQ:
			*valueP = llHdl->cybwIrq[ch];
            break;
        /*--------------------------+
        |   line-break irq enable   |
        +--------------------------*/
        case M72_LBREAK_IRQ:
			*valueP = llHdl->lbreakIrq[ch];
            break;
        /*--------------------------+
        |   xIN2 edge irq enable    |
        +--------------------------*/
        case M72_XIN2_IRQ:
			*valueP = llHdl->xin2Irq[ch];
            break;
        /*--------------------------+
        |   interrupt status        |
        +--------------------------*/
        case M72_INT_STATUS:
		{
			OSS_IRQ_STATE oldState;

			if (llHdl->enbIrq[ch]) {	/* access to shadow register */
				oldState = OSS_IrqMaskR(llHdl->osHdl, llHdl->irqHdl);
				*valueP = (int32)llHdl->regIntStatChan[ch];
				OSS_IrqRestore(llHdl->osHdl, llHdl->irqHdl, oldState);
			}
			else {						/* access to int. stat. reg. */
				switch (ch) {
					case 0:
						*valueP = (int32)(MREAD_D16(llHdl->ma,
													IRQ_STATE_REG1) & 0x001f);
						break;
					case 1:
						*valueP = (int32)((MREAD_D16(llHdl->ma,
													 IRQ_STATE_REG1) & \
										   0x1f00)>>8); break;
					case 2:
						*valueP = (int32)(MREAD_D16(llHdl->ma,
													IRQ_STATE_REG2) & 0x001f);
						break;
					case 3:
						*valueP = (int32)((MREAD_D16(llHdl->ma,
													 IRQ_STATE_REG2) & \
										   0x1f00)>>8); break;
				}
			}
			break;
		}
		/*--------------------------+
        |   comparator A value      |
        +--------------------------*/
        case M72_VAL_COMPA:
			*valueP = llHdl->valCompA[ch];
            break;
        /*--------------------------+
        |   comparator B value      |
        +--------------------------*/
        case M72_VAL_COMPB:
			*valueP = llHdl->valCompB[ch];
            break;
        /*--------------------------+
        |   read mode               |
        +--------------------------*/
        case M72_READ_MODE:
			*valueP = llHdl->readMode[ch];
            break;
        /*--------------------------+
        |   read timeout            |
        +--------------------------*/
        case M72_READ_TIMEOUT:
			*valueP = llHdl->readTimeout[ch];
            break;
        /*--------------------------+
        |   write mode              |
        +--------------------------*/
        case M72_WRITE_MODE:
			*valueP = llHdl->writeMode[ch];
            break;
        /*--------------------------+
        |   timer start condition   |
        +--------------------------*/
        case M72_TIMER_START:
			*valueP = llHdl->timerStart[ch];
            break;
        /*--------------------------+
        |   output signal mode      |
        +--------------------------*/
        case M72_OUT_MODE:
			*valueP = llHdl->regOutCtrl1 | (u_int32)llHdl->regOutCtrl2 << 16;
			break;
		/*--------------------------+
        |   output signal setting  |
        +--------------------------*/
        case M72_OUT_SET:
			*valueP = MREAD_D16(llHdl->ma, OUT_CONFIG_REG) & 0xf;
			break;
        /*--------------------------+
        |   selftest config.        |
        +--------------------------*/
        case M72_SELFTEST:
			*valueP = llHdl->regSelftest;
			break;
        /*--------------------------+
        |  signal code              |
        +--------------------------*/
        case M72_SIGSET_READY:
        case M72_SIGSET_COMP:
        case M72_SIGSET_CYBW:
        case M72_SIGSET_LBREAK:
		case M72_SIGSET_XIN2:
		{
			/* get index */
			u_int32 sigNr = code - (M72_SIGSET);

			/* return signal */
			if (llHdl->sigHdl[ch][sigNr] == NULL)
				*valueP = 0x00;
			else
				OSS_SigInfo(llHdl->osHdl,
							llHdl->sigHdl[ch][sigNr],
							valueP,
							&dummy);

            break;
		}
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/******************************* M72_BlockRead *******************************
 *
 *  Description:  Read a data block from the device
 *
 *                The function is not supported and always returns an
 *                ERR_LL_READ error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGWRT_1((DBH, "LL - M72_BlockRead: ch=%d, size=%d\n",ch,size));

	/* return number of read bytes */
	*nbrRdBytesP = 0;

	return(ERR_LL_READ);
}

/****************************** M72_BlockWrite *******************************
 *
 *  Description:  Write a data block to the device
 *
 *                The function is not supported and always returns an
 *                ERR_LL_WRITE error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
	DBGWRT_1((DBH, "LL - M72_BlockWrite: ch=%d, size=%d\n",ch,size));

	/* return number of written bytes */
	*nbrWrBytesP = 0;

	return(ERR_LL_WRITE);
}


/****************************** M72_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                The interrupt is triggered when the channel interrupt
 *                (IRQEN) is enabled.
 *                Depending on the configuration in the Interrupt Control
 *                Registers, one of the following interrupts will occur:
 *                - Ready
 *                  (cannot be configured in the Interrupt Control Registers,
 *                  see hardware manual)
 *                - Comparator
 *                - Carry/Borrow
 *                - xIN2 Edge
 *                - Line-Break
 *                  When a line break was detected, the interrupt service
 *                  routine disables the Line-Break interrupt
 *                  for the respective channel.
 *                  Line-break detection must be explicitly enabled via
 *                  M72_SetStat (M72_LBREAK_IRQ).
 *
 *                If IRQEN is set in the Interrupt Control Register:
 *                  The shadow registers are updated (ORed with current
 *                  Interrupt Status Register information).
 *                  The pending flags of the Interrupt Status Registers are
 *                  cleared. The function sends the correponding user signals
 *                  if installed and releases a read semaphore when needed.
 *
 *                If IRQEN is not set in the Interrupt Control Register:
 *                  The bits in the Interrupt Status Registers are ignored.
 *                  Shadow registers for Interrupt Status Registers are set
 *                  to zero.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *  Output.....:  return   LL_IRQ_DEVICE	irq caused by device
 *                         LL_IRQ_DEV_NOT   irq not caused by device
 *                         LL_IRQ_UNKNOWN   unknown
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Irq(
   LL_HANDLE *llHdl
)
{
	u_int32 n;
	u_int32 irq_state, bitmask = 0x1f;
	volatile u_int32 preloadB = 0, preloadC = 0;
	volatile u_int32 cntVal = 0, cntLo = 0, cntHi = 0;

	IDBGWRT_1((DBH, "M72_Irq:\n"));

	/*-------------------------------+
	|  read/reset pending irq flags  |
	|  and update shadow registers   |
	+-------------------------------*/
	irq_state = ( (MREAD_D16(llHdl->ma, IRQ_STATE_REG1)) |
		          (u_int32)(MREAD_D16(llHdl->ma, IRQ_STATE_REG2) << 16) );

	for (n=0; n < CH_NUMBER; n++) {
		if (llHdl->enbIrq[n]) {
			/* update shadow reg. */
			llHdl->regIntStatChan[n] |= ( (irq_state >> (n<<3)) & bitmask );
		}
		else {
			llHdl->regIntStatChan[n] = 0;	/* set shadow register = 0 */
			/* irqen = 0 -> irqstate is not relevant now */
			irq_state &= ~(bitmask << (n<<3));
		}
	}

    /* reset irq flags */
	MWRITE_D16(llHdl->ma, IRQ_STATE_REG1, (u_int16)irq_state);
	MWRITE_D16(llHdl->ma, IRQ_STATE_REG2, (u_int16)(irq_state>>16));

	/* no interrupt pending ? */
	if (irq_state == 0)
		return(LL_IRQ_DEV_NOT);		/* say: not caused by device */

	cntLo 	= MREAD_D16(llHdl->ma, COUNT_LOW_REG(0));
	cntHi 	= MREAD_D16(llHdl->ma, COUNT_HIGH_REG(0));
	cntVal 	= cntLo | (cntHi << 16);

	/* 
	 *  Calculate preload Values:
	 *  preload TimerB =   TimerA    - Pretrig 
	 *  preload TimerC =  (TimerA/2) - Pretrig
	 */

	if (llHdl->enPretrig) {
		preloadB = cntVal 		- llHdl->cntPretrig;
		preloadC = (cntVal / 2) - llHdl->cntPretrig;
	} else {
		/*
		 * The user disabled pretriggering since last Irq. To disable 
		 * pretriggering synchronously, we just disable TimerA's
		 * physical IRQ now, making this the last time its serviced. Timers
		 * B and C are reloaded with maximal high dummy Values.
		 */
		preloadB = 0x7fffffff;
		preloadC = 0x7fffffff;

		/*  1. disable IRQ of xIN2*/
		llHdl->xin2Irq[0] = 0;
		llHdl->regIrqCtrl[0] &= ~XIN2_ENB;
		MWRITE_D16(llHdl->ma,IRQ_CTRL_REG(0),llHdl->regIrqCtrl[0]);

		/*  2. disable whole channels IRQ */
		llHdl->enbIrq[0] 		= TRUE;
		llHdl->regIrqCtrl[0] 	&= ~ENB_MASK;
		llHdl->regIrqCtrl[0] 	|= (u_int16)(TRUE << 7);
		MWRITE_D16(llHdl->ma,IRQ_CTRL_REG(0),llHdl->regIrqCtrl[0]);
	}

	IDBGWRT_1((DBH, "Pretrig: %d  cv %08x\n", llHdl->cntPretrig, cntVal ));

#if 0 /* This Modules Registers are writeonly anyway.. */
	IDBGWRT_1((DBH, "Whole M-Modul Address Space:\n"));
	for (n=0; n < 0x54; n+=2) {
		if(!(n % 16))
			IDBGWRT_1((DBH, "\n"));		
		IDBGWRT_1((DBH, "%04x ", MREAD_D16(llHdl->ma, n) ));
	}

#endif

	/* update both TimerB and C simultaneous */
	MWRITE_D16(llHdl->ma, PRELOAD_LOW_REG(PRETR_TIMER_B),
			   (u_int16)(preloadB & 0xffff));
	MWRITE_D16(llHdl->ma, PRELOAD_HIGH_REG(PRETR_TIMER_B),
			   (u_int16)(preloadB >> 16));

	MWRITE_D16(llHdl->ma, PRELOAD_LOW_REG(PRETR_TIMER_C),
			   (u_int16)(preloadC & 0xffff));
	MWRITE_D16(llHdl->ma, PRELOAD_HIGH_REG(PRETR_TIMER_C),
			   (u_int16)(preloadC >> 16));

#if DBG
	/* print pending flags */
	for (n=0; n < CH_NUMBER; n++) {
		if (llHdl->enbIrq[n]) {
			IDBGWRT_2((DBH," %d=%c%c%c%c%c", n,
					   (irq_state & READY_PEND(n)  ? 'R':'.'),
					   (irq_state & COMP_PEND(n)   ? 'C':'.'),
					   (irq_state & CYBW_PEND(n)   ? 'Y':'.'),
					   (irq_state & LBREAK_PEND(n) ? 'L':'.'),
					   (irq_state & XIN2_PEND(n)   ? 'X':'.')));
		}
	}
	DBGWRT_2((DBH,"\n"));
#endif

#if 0 /* Signaling not used in this customer specific driver */
	/*------------------------------------------------------+
	|  if IRQEN = 1 for channel: send signals if installed  |
	+------------------------------------------------------*/
	for (n=0; n<CH_NUMBER; n++) {
		if (llHdl->enbIrq[n]) {
			/* Ready  irq ? */
			if (irq_state & READY_PEND(n)) {
				/* send signal if installed */
				if (llHdl->sigHdl[n][0]) {
					OSS_SigSend(llHdl->osHdl, llHdl->sigHdl[n][0]);
				}
				/* handle read mode */
				if (llHdl->readMode[n] == M72_READ_WAIT) {
					OSS_SemSignal(llHdl->osHdl, llHdl->readSemHdl[n]);
				}
			}

			/* Comparator irq ? */
			if ((irq_state & COMP_PEND(n)) && llHdl->sigHdl[n][1])
				OSS_SigSend(llHdl->osHdl, llHdl->sigHdl[n][1]);

			/* Carry/Borrow irq ? */
			if ((irq_state & CYBW_PEND(n)) && llHdl->sigHdl[n][2])
				OSS_SigSend(llHdl->osHdl, llHdl->sigHdl[n][2]);

			/* Line-Break irq ? */
			if (irq_state & LBREAK_PEND(n)) {
				/* send signal if installed */
				if (llHdl->sigHdl[n][3]) {
					OSS_SigSend(llHdl->osHdl, llHdl->sigHdl[n][3]);
				}
				/* disable Line-Break irq */
				llHdl->lbreakIrq[n] = 0;
				llHdl->regIrqCtrl[n] &= ~LBREAK_ENB;
				MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(n), llHdl->regIrqCtrl[n]);
			}

			/* xIN2 Edge irq ? */
			if ((irq_state & XIN2_PEND(n)) && llHdl->sigHdl[n][4])
				OSS_SigSend(llHdl->osHdl, llHdl->sigHdl[n][4]);
		}
	}
#endif

	llHdl->irqCount++;
	return(LL_IRQ_DEVICE);		/* say: caused by device */
}

/****************************** M72_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements.
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and
 *                data modes (ORed) which are supported by the hardware
 *                (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used by the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *                data mode represents the widest hardware access used by
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns whether the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns which process locking
 *                mode the driver needs (LL_LOCK_xxx).
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *  Output.....:  return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M72_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)    |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process locking mode         |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )  /* nodoc */
{
    return( "M72 - M72 low-level driver: $Id: m72_drv_pretrig.c,v 1.6 2010/04/20 15:10:39 amorbach Exp $" );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, deinstall all installed signals and
 *               semaphores, free memory and return error code
 *		         NOTE: The low-level handle is invalid after this function is
 *                     called.
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl		low-level handle
 *               retCode    return value
 *  Output.....: return	    retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode		/* nodoc */
)
{
	u_int32 n, sigNr;

    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up microwire handle */
	if (llHdl->mcrwHdl) {
		llHdl->mcrwHdl->Exit((void**)&llHdl->mcrwHdl);
	}

	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up signals */
	for (n=0; n<CH_NUMBER; n++) {
		for (sigNr=0; sigNr<SIG_COUNT; sigNr++) {
			if (llHdl->sigHdl[n][sigNr])
				OSS_SigRemove(llHdl->osHdl, &llHdl->sigHdl[n][sigNr]);
		}
	}

	/* clean up semaphores */
	for (n=0; n<CH_NUMBER; n++) {
		OSS_SemRemove(llHdl->osHdl, &llHdl->readSemHdl[n]);
	}

	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}

/********************************* CounterStore ******************************
 *
 *  Description: Config/Force counter store (latch) on given channel
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl	  low-level handle
 *               ch       channel number
 *               cond     counter store condition (M72_STORE_x)
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void CounterStore(
   LL_HANDLE    *llHdl,
   int32        ch,
   int32        cond	/* nodoc */
)
{
	u_int16 ctrl;

	/*---------------------+
	| force counter store  |
	+---------------------*/
	if (cond == M72_STORE_NOW) {
		DBGWRT_2((DBH, " counter %d latch\n",ch));

		/* force latch */
		ctrl = (u_int16)((llHdl->regCountCtrl[ch] & ~STORE_MASK) | \
						 (M72_STORE_NOW << 4));
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), ctrl);

		/* restore config */
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
	/*---------------------+
	| config counter store |
	+---------------------*/
	else {
		llHdl->cntStore[ch] = cond;
		llHdl->regCountCtrl[ch] &= ~STORE_MASK;
		llHdl->regCountCtrl[ch] |= (u_int16)(cond << 4);
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
}

/********************************* CounterClear *****************************
 *
 *  Description: Config/Force counter clear on given channel
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl	  low-level handle
 *               ch       channel number
 *               cond     counter clear condition (M72_CLEAR_x)
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void CounterClear(
   LL_HANDLE    *llHdl,
   int32        ch,
   int32        cond	/* nodoc */
)
{
	u_int16 ctrl;

	/*---------------------+
	| force counter clear  |
	+---------------------*/
	if (cond == M72_CLEAR_NOW) {
		DBGWRT_2((DBH, " counter %d clear\n",ch));

		/* force clear */
		ctrl =(u_int16)(llHdl->regCountCtrl[ch] & ~CLEAR_MASK)|(M72_CLEAR_NOW);
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), ctrl);

		/* restore config */
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
	/*---------------------+
	| config counter clear |
	+---------------------*/
	else {
		llHdl->cntClear[ch] = cond;
		llHdl->regCountCtrl[ch] &= ~CLEAR_MASK;
		llHdl->regCountCtrl[ch] |= (u_int16)(cond << 0);
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
}

/********************************* CounterLoad ******************************
 *
 *  Description: Config/Force counter load on given channel
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl	  low-level handle
 *               ch       channel number
 *               cond     counter store condition (M72_STORE_x)
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void CounterLoad(
   LL_HANDLE    *llHdl,
   int32        ch,
   int32        cond	/* nodoc */
)
{
	u_int16 ctrl;

	/*---------------------+
	| force counter load   |
	+---------------------*/
	if (cond == M72_PRELOAD_NOW) {
	    DBGWRT_2((DBH, " counter %d load\n",ch));

		/* force load */
		ctrl = (u_int16)((llHdl->regCountCtrl[ch] & ~PRELOAD_MASK) | \
						 (M72_PRELOAD_NOW << 2));
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), ctrl);

		/* restore config */
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
	/*---------------------+
	| config counter load  |
	+---------------------*/
	else {
		llHdl->cntPreload[ch] = cond;
		llHdl->regCountCtrl[ch] &= ~PRELOAD_MASK;
		llHdl->regCountCtrl[ch] |= (u_int16)(cond << 2);
		MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(ch), llHdl->regCountCtrl[ch]);
	}
}

/********************************* CounterLoad ******************************
 *
 *  Description: Helper to setup TimerA/B/C for automatic pretrigger generation
 *
 *---------------------------------------------------------------------------
 *  Input......: llHdl	  low-level handle
 *
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void TimerABCsetup( LL_HANDLE *llHdl)
{

	/*--------------------+
	 |  TimerA configure  |
	 +--------------------*/

	/* Preload with 0 */
	llHdl->valPreload[0] = 0;
	MWRITE_D16(llHdl->ma, PRELOAD_LOW_REG(0), 	0);
	MWRITE_D16(llHdl->ma, PRELOAD_HIGH_REG(0), 	0);

	/* set COMP_A Value to 0xffffffff */
	llHdl->valCompA[0] = 0xffffffff;
	MWRITE_D16(llHdl->ma, COMPA_HIGH_REG(0), 	0xffff);
	MWRITE_D16(llHdl->ma, COMPA_LOW_REG(0),  	0xffff);

	/* Set OUT_0=1 to let TimerA continously count upwards */
	MWRITE_D16(llHdl->ma, OUT_CONFIG_REG, (u_int16)0x1);

	/*
	 *  Direct combined bit setting of CCRA:
	 *   - latch Value on xIN2+
	 *   - clear on xIN2+ rising edge
 	 *   - counter mode: Timer Mode
	 *  Property
     *                  Timer Mode      Sta Tim Latch   No Pre- clear
     * |               |               |rt |bas|on xIN2|load   |on xIN2|
     * ----------------------------------------------------------------|
     * | - | - | - | - | 1 | 0 | 1 | 0 | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 1 |
     * |       0       |       A       |       A       |       1       |
	 */
	MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(0), 0x0aa1);

	/*--------------------+
	 |  TimerB configure  |
	 +--------------------*/

	/*
	 * Set Comp_A = 0 because TimerC decrements when xIN1
	 * is low ( no Vcc is attached to it on DSUB connector)
	 */
	llHdl->valCompA[1] = 0x0;
	MWRITE_D16(llHdl->ma, COMPA_LOW_REG(1),		0 );
	MWRITE_D16(llHdl->ma, COMPA_HIGH_REG(1), 	0 );

	/* enable Comparator IRQ (only internally, not causing IRq on MM IF) */
	llHdl->regIrqCtrl[1] = M72_COMP_EQUAL << 4;
	MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(1), llHdl->regIrqCtrl[1]);
	
	/* counter operating mode: Timer preload on xIN2+, Start at xIN2 edge
     *                  Timer Mode      Sta Tim No      Preload No
     * |               |               |rt |bas|latch  |on xIN2|clear  |
     * ----------------------------------------------------------------|
     * | - | - | - | - | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |
     * |       0       |       A       |       0       |       4       |
	 */
	MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(1), 0x0a04);

	/*--------------------+
	 |  TimerC configure  |
	 +--------------------*/

	/*
	 * Set Comp_A = 0 because TimerC decrements when xIN1
	 * is low ( no Vcc is attached to it on DSUB connector)
	 */
	llHdl->valCompA[2] = 0;
	MWRITE_D16(llHdl->ma, COMPA_LOW_REG(2),	 0 );
	MWRITE_D16(llHdl->ma, COMPA_HIGH_REG(2), 0 );

	/* enable Comparator IRQ (only internally, not causing IRq on MM IF) */
	llHdl->regIrqCtrl[2] = M72_COMP_EQUAL << 4;
	MWRITE_D16(llHdl->ma, IRQ_CTRL_REG(2), llHdl->regIrqCtrl[2]);


	/*  counter operating mode: Timer
	 *  preloaded on xIN2+, Start at xIN2 edge
     *                  Timer Mode      Sta Tim No      Preload No
     * |               |               |rt |bas|latch  |on xIN2|clear  |
     * ----------------------------------------------------------------|
     * | - | - | - | - | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |
     * |       0       |       A       |       0       |       4       |
	 */
	MWRITE_D16(llHdl->ma, COUNT_CTRL_REG(2), 0x0a04);


	/*--------------------+
	 | Compose pretrig Sig|
	 +--------------------*/

	/*
	 * generate 50% duty cycle output on OUT 4 by superposition of
	 * Timer B and C: set OUT4 if TimerB = 0, clear if TimerC = 0
	 * In binary: 00 10 01 00 = 0x24nn nnnn (see User manual)
	 */
	llHdl->regOutCtrl1 = (u_int16)(0x0000);
	llHdl->regOutCtrl2 = (u_int16)(0x2400);
	MWRITE_D16(llHdl->ma, OUT_CTRL1_REG, llHdl->regOutCtrl1);
	MWRITE_D16(llHdl->ma, OUT_CTRL2_REG, llHdl->regOutCtrl2);
#if 0
	DBGWRT_1((DBH,"timerABCsetup finished. CCRA %04x CCRB %04x CCRC %04x\n",
			  MREAD_D16(llHdl->ma, COUNT_CTRL_REG(0)),
			  MREAD_D16(llHdl->ma, COUNT_CTRL_REG(1)),
			  MREAD_D16(llHdl->ma, COUNT_CTRL_REG(2)) ));

	DBGWRT_1((DBH," ICRA %04x ICRB %04x ICRC %04x\n",
			  MREAD_D16(llHdl->ma, IRQ_CTRL_REG(0)),
			  MREAD_D16(llHdl->ma, IRQ_CTRL_REG(1)),
			  MREAD_D16(llHdl->ma, IRQ_CTRL_REG(2)) ));
#endif

}


