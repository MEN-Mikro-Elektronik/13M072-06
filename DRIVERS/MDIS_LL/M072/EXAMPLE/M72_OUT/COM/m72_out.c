/****************************************************************************
 ************                                                    ************
 ************                   M72_OUT                          ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: see
 *        $Date: 2010/04/20 15:04:08 $
 *    $Revision: 1.7 $
 *
 *  Description: M72 example for output setting
 *                      
 *               Configuration:
 *               - (none)
 *               
 *               Output:
 *               - Toggle Control_1..4 Outputs
 *               
 *     Required: MDIS user interface library
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_out.c,v $
 * Revision 1.7  2010/04/20 15:04:08  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.6  2006/09/27 18:55:24  ts
 * changed return type of main() to int due to Compiler warnings
 *
 * Revision 1.5  2004/08/30 12:19:26  dpfeuffer
 * minor modifications for MDIS4/2004 conformity
 *
 * Revision 1.4  1999/08/06 15:07:08  Franke
 * cosmetics
 *
 * Revision 1.3  1999/08/06 10:30:16  Schoberl
 * added headers stdlib.h, string.h
 *
 * Revision 1.2  1999/08/06 09:19:18  Schoberl
 * cosmetics
 * functions static
 * main directly called in all systems now
 *
 * Revision 1.1  1998/10/23 16:31:26  see
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
 *  Output.....: return         success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc,char *argv[])
{
	MDIS_PATH path=0;
    int32 cnt,outSet;
	char *device;

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m72_out <device>\n");
		printf("Function: M72 example for output setting\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("\n");
		return(1);
	}

    device = argv[1];
	
	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*--------------------+
    |  output loop        |
    +--------------------*/
	printf("Configuration:\n");
	printf("- (none)\n");
	printf("\n");
	printf("Output:\n");
	printf("- Toggle Control_1..4 Outputs\n");
	printf("(Press any key for exit)\n");
	printf("\n");

	cnt = 0;
	outSet = 0;
	
	do {
		/* toggle outSet */
		switch(cnt++ & 0x03) {
			case 0: 	outSet = M72_OUT_SET_CTRL1;	break;
			case 1: 	outSet = M72_OUT_SET_CTRL2;	break;
			case 2: 	outSet = M72_OUT_SET_CTRL3;	break;
			case 3: 	outSet = M72_OUT_SET_CTRL4;	break;
		}

		/* output signal setting */
		if ((M_setstat(path, M72_OUT_SET, outSet)) < 0) {
			PrintError("setstat  M72_OUT_SET");
			goto abort;
		}

		/* print result */
		printf("output=0x%02lx\n", outSet);

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




 
