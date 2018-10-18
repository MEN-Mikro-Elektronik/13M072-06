/****************************************************************************
 ************                                                    ************
 ************                   M72_PRETRIG                       ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ts
 *        $Date: 2010/04/20 15:04:21 $
 *    $Revision: 1.7 $
 *
 *  Description: M72 example for IRQ context initialized pretriggering
 *                      
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_pretrig.c,v $
 * Revision 1.7  2010/04/20 15:04:21  amorbach
 * R:1. Porting to MDIS5
 * 2. Warning: Unnecessary export of symbol usage
 * M:1. changed according to MDIS Porting Guide 0.8
 * 2. usage() changed to static
 *
 * Revision 1.6  2008/02/26 17:09:59  ts
 * added automatic option to continously en/disable pretriggering
 * complete initialization is done in driver by Setstat EN_PRETRIG
 *
 * Revision 1.5  2007/07/31 17:29:52  ts
 * works with direct clockcounting between 2 xIN2+ edges
 *
 * Revision 1.4  2007/07/31 11:34:15  ts
 * Added Tests for simultanoeous clear/restart of TimerA
 *
 * Revision 1.3  2007/05/24 17:54:08  ts
 * Cosmetics, CHK Macro
 *
 * Revision 1.2  2006/09/27 18:55:37  ts
 * changed return type of main() to int due to Compiler warnings
 *
 * Revision 1.1  2006/06/30 19:00:13  ts
 * Initial Revision
 *
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

#define CHK(expression) \
       if((expression)) {\
           printf("\n*** Error during: %s\nfile %s\nline %d\n", \
            #expression,__FILE__,__LINE__);\
            printf("%s\n",M_errstring(UOS_ErrnoGet()));\
           goto abort;\
       }

/* how often to toggle Pretrigger en/disabling? */
#define KEYPRESS_COUNT 10
      
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintError(char *info);


static void usage(void)
{

	printf("Syntax: m72_pretrig <device> <meth> <-u=> <-a>\n");
	printf("Function:\nExample for Interrupt-context initiated trigger\n");
	printf("This can be used to pretrigger a Scope on events\n\n");
	printf("                    Npretrig\n");
	printf("                         --->.  |<---\n");
	printf("   |_                        .  |_\n");
	printf("   | |        Ncycle-Npretrig.  | |\n");
	printf("   |-|---------------------->.  | |\n");
	printf(" __| |_______________________.__| |___ TTL on Chan.A\n");
	printf("        Ncycle               .\n");
	printf("   |--------------------------->|\n\n");
	printf("Options:\n");
	printf("    device       device name\n");
	printf("    chan (0=Chan. A .. 3=Chan. C), !!Currently always Chan.A!\n");
	printf("   -u=<Npretrig> in 0,4 us(1/2,5MHz) units (0..n, decimal)\n");
	printf("   -a if given, pretrigger switches on/off automatically\n");
	printf("\n");
}


/********************************* main *************************************
 *
 *  Description: MAIN entry
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		command line arguments/counter
 *  Output.....: return         success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH path=0;
#if 0
	int32 chan, meth;
#endif
	u_int32 bPreEnable = 1, automode = 0;
	u_int32 valPreload = 0;
	u_int32 nKeypressCount = 0;   /* test Pretrigger Generation Gating */
	char *device,*str;

	printf("Pretrigger test simulator\n");

	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		usage();
		return(1);
	}

	valPreload= ((str = UTL_TSTOPT("u=")) ? atoi(str) : 0);
	automode = ((str = UTL_TSTOPT("a")) ? 1 : 0);

	if(automode)
		printf("Automatic en/disabling\n");

	device 	= argv[1];
#if 0
	chan 	= 0;
	meth 	= atoi(argv[2]);
#endif

	/*--------------------+
	 |  open path         |
	 +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}



	/* enable global interrupt */
	CHK((M_setstat(path, M_MK_IRQ_ENABLE, 1)) < 0);

	/* 
	 * set pretrigger Value in multiples of 1 / 2,5 MHz (=0,4 us)
	 * this corresponds to llHdl->cntPretrig[1] in driver ! 
	 */
	CHK((M_setstat(path, M72_CNT_PRETRIG, valPreload )) < 0);

	bPreEnable = 1;
	CHK((M_setstat(path, M72_EN_PRETRIG, 0x1 )) < 0);


	/*--------------------+
	 |  measurement loop  |
	 +--------------------*/

	/* set Channel to chan A  again */
	/* CHK((M_setstat(path, M_MK_CH_CURRENT, 0 )) < 0); */

	if (automode) {

		while(1){
			if (bPreEnable) {
				printf("switch Pretrig OFF\n");
				bPreEnable = 0;
			} else {
				printf("switch Pretrig ON\n");
				bPreEnable = 1;
			}
			CHK((M_setstat(path, M72_EN_PRETRIG, bPreEnable )) < 0);
			/* wait some time */
			UOS_Delay( 1730 );
		}	
		
	} else {
		printf("Press any key several times for toggling"
			   " Pretrig Generation\n");
		do {
			if( UOS_KeyPressed() >= 0) {
				nKeypressCount++;
				/* toggle Pretrigger Generation */
				if (bPreEnable) {
					printf("switch Pretrig OFF\n");
					bPreEnable = 0;
				} else {
					printf("switch Pretrig ON\n");
					bPreEnable = 1;
				}
				CHK((M_setstat(path, M72_EN_PRETRIG, bPreEnable )) < 0);
				/* wait some time */
				UOS_Delay( LOOPDELAY );
			}
	
		} while( (!automode) && (nKeypressCount < KEYPRESS_COUNT) );
	}

	/*--------------------+
	 |  cleanup            |
	 +--------------------*/

	/* irq disble for channel */
	CHK((M_setstat(path, M72_ENB_IRQ, FALSE)) < 0);

	/* disable interrupt */
	CHK((M_setstat(path, M_MK_IRQ_ENABLE, 0)) < 0);


 abort:
	if (M_close(path) < 0)
		PrintError("close");

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






 
