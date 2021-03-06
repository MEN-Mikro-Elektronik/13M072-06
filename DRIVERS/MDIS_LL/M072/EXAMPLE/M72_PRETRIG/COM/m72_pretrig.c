/****************************************************************************
 ************                                                    ************
 ************                   M72_PRETRIG                       ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ts
 *
 *  Description: M72 example for IRQ context initialized pretriggering
 *                      
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
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






 
