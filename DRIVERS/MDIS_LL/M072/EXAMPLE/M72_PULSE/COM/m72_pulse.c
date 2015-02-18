/****************************************************************************
 ************                                                    ************
 ************                   M72_PULSE                        ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *        $Date: 2010/04/20 15:04:26 $
 *    $Revision: 1.7 $
 *
 *  Description: M72 example for pulse width measurement
 *                      
 *               Configuration:
 *               - pulse width high measurement
 *               - read mode: wait for ready before read
 *               
 *               Measurement:
 *               - start measurement by counter clear
 *               - read counter of channel 0
 *               
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_pulse.c,v $
 * Revision 1.7  2010/04/20 15:04:26  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.6  2006/09/27 18:55:44  ts
 * changed return type of main() to int due to Compiler warnings
 *
 * Revision 1.5  2004/08/30 12:19:35  dpfeuffer
 * minor modifications for MDIS4/2004 conformity
 *
 * Revision 1.4  1999/08/06 15:07:15  Franke
 * cosmetics
 *
 * Revision 1.3  1999/08/06 10:30:21  Schoberl
 * added headers stdlib.h, string.h
 *
 * Revision 1.2  1999/08/06 09:19:23  Schoberl
 * cosmetics
 * functions static
 * main directly called in all systems now
 * added:
 * -setstat for IRQ enable per channel
 *
 * Revision 1.1  1998/10/23 16:31:37  see
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
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintError(char *info);

/********************************* main *************************************
 *
 *  Description: MAIN entry 
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		command line arguments/counter
 *  Output.....: return    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH path=0;
    int32 chan;
	u_int32 count;
	char *device;
	
	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m72_pulse <device> <chan>\n");
		printf("Function: M72 example for pulse width measurement\n");
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

	/* counter mode: pulse width high measurment */
	if ((M_setstat(path, M72_CNT_MODE, M72_MODE_PULSEH)) < 0) {
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
	printf("- pulse width high measurement\n");
	printf("- read mode: wait for ready before read\n");
	printf("\n");
	printf("Measurement:\n");
	printf("- start measurement by counter clear\n");
	printf("- read counter of channel %ld\n", chan);
	printf("(Press any key for exit)\n");
	printf("\n");

	do {
		/* start measurement */
		printf("start pulse width measurement\n");

		if (M_setstat(path, M72_CNT_CLEAR, M72_CLEAR_NOW) < 0) {
			PrintError("setstat M72_CNT_CLEAR");
			goto abort;
		}

		/* read counter */
		if (M_read(path, (int32*)&count) < 0) {
			PrintError("read");
			break;
		}

		/* print result */
		printf("counter=0x%08lx => time=%f sec\n\n",
			   count, (float)count / 2500000);

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




