/****************************************************************************
 ************                                                    ************
 ************                   M72_SINGLE                       ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *
 *  Description: M72 example for single count mode
 *
 *               Configuration:
 *               - single counter mode
 *               - signal on comparator match (0x1000)
 *               - clear counter with comparator match
 *               
 *               Measurement:
 *               - latch and read counter of channel 0
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

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void __MAPILIB SigHandler( u_int32 sigCode );
static void PrintError(char *info);

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int16 G_compMatch = FALSE, G_unkwnSig = FALSE;
static u_int32 G_signalCode;

/********************************* SigHandler *******************************
 *
 *  Description: Signal handler
 *			   
 *---------------------------------------------------------------------------
 *  Input......: sigCode	signal code received
 *  Output.....: -
 *  Globals....: G_compMatch, G_unkwnSig, G_signalCode
 ****************************************************************************/

static void __MAPILIB SigHandler( u_int32 sigCode )
{
	switch (sigCode) 
	{
		case UOS_SIG_USR1:
			G_compMatch = TRUE;
			break;
		default:
			G_signalCode = sigCode;
			G_unkwnSig = TRUE;
	}
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
int main(int argc,char *argv[])
{
	MDIS_PATH path=0;
    int32 error;
	int32 chan;
	u_int32 count, usrSig = FALSE;
	char *device;

	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m72_single <device> <chan>\n");
		printf("Function: M72 example for single count mode\n");
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

	/* comparator A value (0x1000) */
	if ((M_setstat(path, M72_VAL_COMPA, 0x1000)) < 0) {
		PrintError("setstat M72_VAL_COMPA");
		goto abort;
	}

	/* counter mode: single count */
	if ((M_setstat(path, M72_CNT_MODE, M72_MODE_SINGLE)) < 0) {
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

	/* irq enable for channel */
	if ((M_setstat(path, M72_ENB_IRQ, TRUE)) < 0) {
		PrintError("setstat M72_ENB_IRQ");
		goto abort;
	}

	/* install comparator signal */
	if ((M_setstat(path, M72_SIGSET_COMP, UOS_SIG_USR1)) < 0) {
		PrintError("setstat M72_SIGSET_COMP");
		goto abort;
	}
	usrSig = TRUE;

	/* enable interrupt */
	if ((M_setstat(path, M_MK_IRQ_ENABLE, 1)) < 0) {
		PrintError("setstat M_MK_IRQ_ENABLE");
		goto abort;
	}

	/*--------------------+
    |  measurement loop   |
    +--------------------*/
	printf("Configuration:\n");
	printf("- single counter mode\n");
	printf("- signal on comparator match (0x1000)\n");
	printf("- clear counter with comparator match\n");
	printf("- read mode: latch before read\n");
	printf("\n");
	printf("Measurement:\n");
	printf("- read counter of channel %ld\n", chan);
	printf("- wait for signals\n");
	printf("(Press any key for exit)\n");
	printf("\n");

	do {

		if (G_compMatch) {
			printf(">>> comparator match occured\n");
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




 
