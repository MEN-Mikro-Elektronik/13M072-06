/****************************************************************************
 ************                                                    ************
 ************                   M72_FREQ                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *        $Date: 2010/04/20 15:04:03 $
 *    $Revision: 1.7 $
 *
 *  Description: M72 example for frequency measurement
 *                      
 *               Configuration:
 *               - frequency measurement mode
 *               - read mode: wait for ready before read
 *               
 *               Measurement:
 *               - start measurement
 *               - read counter of channel
 *               
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany 
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
static void PrintError(char *info);

/********************************* main *************************************
 *
 *  Description: MAIN entry
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv	   command line arguments/counter
 *  Output.....: return	       success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH path=0;
    int32 chan;
	u_int32 count;
	char *device;
	

	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m72_freq <device> <chan>\n");
		printf("Function: M72 example for frequency measurement\n");
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
    |  config             |
    +--------------------*/
	/* channel number */
	if ((M_setstat(path, M_MK_CH_CURRENT, chan)) < 0) {
		PrintError("setstat M_MK_CH_CURRENT");
		goto abort;
	}

	/* read mode: wait for ready before read */
	if ((M_setstat(path, M72_READ_MODE, M72_READ_WAIT)) < 0) {
		PrintError("setstat M72_READ_MODE");
		goto abort;
	}

	/* counter mode: frequency measurment */
	if ((M_setstat(path, M72_CNT_MODE, M72_MODE_FREQ)) < 0) {
		PrintError("setstat M72_CNT_MODE");
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
	printf("- frequency measurement mode\n");
	printf("- read mode: wait for ready before read\n");
	printf("\n");
	printf("Measurement:\n");
	printf("- start measurement\n");
	printf("- read counter of channel %ld\n", chan);
	printf("(Press any key for exit)\n");
	printf("\n");

	do {
		/* start measurement */
		printf("start freq measurement\n");

		if (M_setstat(path, M72_FREQ_START, 0) < 0) {
			PrintError("setstat M72_FREQ_START");
			goto abort;
		}

		/* read counter */
		if (M_read(path, (int32*)&count) < 0) {
			PrintError("read");
			break;
		}

		/* print result */
		printf("counter=0x%08lx => freq=%ld Hz\n\n", count, count * 100);

		/* wait some time */
		UOS_Delay(LOOPDELAY);

	} while(UOS_KeyPressed() == -1);	/* while no key pressed */

	/*--------------------+
    |  cleanup            |
    +--------------------*/
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




 
