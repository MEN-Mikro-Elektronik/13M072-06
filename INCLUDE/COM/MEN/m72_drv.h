/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m72_drv.h
 *
 *       Author: see
 *        $Date: 2010/04/20 15:11:07 $
 *    $Revision: 2.7 $
 *
 *  Description: Header file for M72 driver
 *               - M72 specific status codes
 *               - M72 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_drv.h,v $
 * Revision 2.7  2010/04/20 15:11:07  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 2.6  2007/05/25 11:58:36  ts
 * + SetStat EN_PRETRIG
 *
 * Revision 2.5  2006/09/18 11:04:37  ts
 * Set SetStat code CNT_PRETRIG to the correct next free Value
 *
 * Revision 2.4  2006/07/01 10:57:59  ts
 * + Setstat M72_CNT_PRETRIG (intermediate checkin)
 *
 * Revision 2.3  2002/03/19 10:54:22  Schoberl
 * cosmetics, support for swapped variant
 *
 * Revision 2.2  1999/08/06 09:19:33  Schoberl
 * cosmetics
 * fixed:
 * - M72 output signal mode macro
 * added:
 * - declarations for ll-functions
 * - variant for swapped access possible
 * - M72_READ_TIMEOUT
 * - M72_SIGSET_XIN2
 * - M72_SIGCLR_XIN2
 * - M72_INT_STATUS
 * removed:
 * - now needless read/write flags
 * - defines for counter mode 8
 * changed:
 * - M72_OUTCFG macro
 * - read mode flags not ORed anymore
 * - write mode flags not ORed anymore
 *
 * Revision 2.1  1998/10/23 16:31:59  see
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _M72_DRV_H
#define _M72_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* M72 specific status codes (STD) */      /* S,G: S=setstat, G=getstat 	 */
#define M72_CNT_MODE    	M_DEV_OF+0x00  /* G,S: counter mode 			 */
#define M72_CNT_PRELOAD 	M_DEV_OF+0x01  /* G,S: counter preload condition */
#define M72_CNT_CLEAR   	M_DEV_OF+0x02  /* G,S: counter clear condition 	 */
#define M72_CNT_STORE   	M_DEV_OF+0x03  /* G,S: counter store condition 	 */
#define M72_COMP_IRQ    	M_DEV_OF+0x04  /* G,S: comparator irq condition  */
#define M72_CYBW_IRQ    	M_DEV_OF+0x05  /* G,S: carry/borrow irq cond. 	 */
#define M72_LBREAK_IRQ  	M_DEV_OF+0x06  /* G,S: line-break irq enable 	 */
#define M72_XIN2_IRQ   		M_DEV_OF+0x07  /* G,S: xIN2 edge  irq enable 	 */
#define M72_VAL_COMPA   	M_DEV_OF+0x08  /* G,S: comparator A value 		 */
#define M72_VAL_COMPB   	M_DEV_OF+0x09  /* G,S: comparator B value 		 */
#define M72_READ_MODE   	M_DEV_OF+0x0a  /* G,S: mode for read calls 		 */
#define M72_WRITE_MODE   	M_DEV_OF+0x0b  /* G,S: mode for write calls 	 */
#define M72_TIMER_START   	M_DEV_OF+0x0c  /* G,S: timer start condition 	 */
#define M72_FREQ_START   	M_DEV_OF+0x0d  /*   S: start frequency measuring */
#define M72_OUT_MODE  	 	M_DEV_OF+0x0e  /* G,S: output signal mode 		 */
#define M72_OUT_SET  	 	M_DEV_OF+0x0f  /* G,S: output signal setting 	 */

#define M72_SIGSET			M_DEV_OF+0x10
#define M72_SIGSET_READY	M72_SIGSET+0x00 /* G,S: install ready signal 	*/
#define M72_SIGSET_COMP		M72_SIGSET+0x01 /* G,S: inst. compare signal 	*/
#define M72_SIGSET_CYBW		M72_SIGSET+0x02 /* G,S: inst. carry/borrow sig. */
#define M72_SIGSET_LBREAK	M72_SIGSET+0x03 /* G,S: inst. linebreak signal	*/
#define M72_SIGSET_XIN2  	M72_SIGSET+0x04 /* G,S: install xIN2 signal 	*/


#define M72_SIGCLR			M_DEV_OF+0x20
#define M72_SIGCLR_READY	M72_SIGCLR+0x00 /*   S: deinstall xIN2/ready Sig */
#define M72_SIGCLR_COMP		M72_SIGCLR+0x01 /*   S: deinstall comparator signal */
#define M72_SIGCLR_CYBW		M72_SIGCLR+0x02 /*   S: deinstall carry/borrow signal */
#define M72_SIGCLR_LBREAK	M72_SIGCLR+0x03 /*   S: deinstall line-break signal */
#define M72_SIGCLR_XIN2  	M72_SIGCLR+0x04 /*   S: deinstall xIN2 signal */

#define M72_SELFTEST  	 	M_DEV_OF+0x30   /* G,S: selftest configuration */

#define M72_READ_TIMEOUT	M_DEV_OF+0x40	/* G,S: read timeout */
#define M72_ENB_IRQ			M_DEV_OF+0x41	/* G,S: global interrupt enable  */
#define M72_INT_STATUS      M_DEV_OF+0x42   /* G,S: IRQ status register 	 */
#define M72_CNT_PRETRIG		M_DEV_OF+0x43 	/* G,S: Timer[0123] reload Val 	 */
#define M72_EN_PRETRIG 		M_DEV_OF+0x44 	/* G,S: enable Pretrg within IRQ */

/* M72 counter modes */
#define M72_MODE_NO	   		0x00		/* no count (halted) */
#define M72_MODE_SINGLE		0x01		/* single count */
#define M72_MODE_1XQUAD		0x02		/* 1x quadrature count */
#define M72_MODE_2XQUAD		0x03		/* 2x quadrature count **/
#define M72_MODE_4XQUAD		0x04		/* 4x quadrature count **/
#define M72_MODE_FREQ		0x05		/* frequency measurement */
#define M72_MODE_PULSEH		0x06		/* pulse width high measurement */
#define M72_MODE_PULSEL		0x07		/* pulse width low measurement */
#define M72_MODE_PERIOD		0x09		/* period measurement */
#define M72_MODE_TIMER		0x0a		/* timer mode */

/* M72 counter preload conditions */
#define M72_PRELOAD_NO		0x00		/* no condition (disabled) */
#define M72_PRELOAD_IN2		0x01		/* at xIN2 rising edge */
#define M72_PRELOAD_NOW		0x02		/* at now (immediately) */
#define M72_PRELOAD_COMP	0x03		/* at comparator match */

/* M72 counter clear conditions */
#define M72_CLEAR_NO		0x00		/* no condition (disabled) */
#define M72_CLEAR_IN2		0x01		/* at xIN2 rising edge */
#define M72_CLEAR_NOW		0x02		/* at now (immediately) */
#define M72_CLEAR_COMP		0x03		/* at comparator match */

/* M72 counter store conditions */
#define M72_STORE_NO		0x00		/* no condition (disabled) */
#define M72_STORE_NOW		0x01		/* at now (immediately) */
#define M72_STORE_IN2		0x02		/* at xIN2 rising edge */

/* M72 comparator interrupt condition */
#define M72_COMP_NO			0x00		/* no condition (disabled) */
#define M72_COMP_LESS		0x01		/* if counter < COMPA */
#define M72_COMP_GREATER	0x02		/* if counter > COMPA */
#define M72_COMP_EQUAL		0x03		/* if counter = COMPA */
#define M72_COMP_INRANGE	0x04		/* if COMPA < counter < COMPB */
#define M72_COMP_OUTRANGE	0x05		/* if counter < COMPA or */
										/*    counter > COMPB */
/* M72 carry/borrow interrupt condition */
#define M72_CYBW_NO			0x00		/* no condition (disabled) */
#define M72_CYBW_CY			0x01		/* if carry */
#define M72_CYBW_BW			0x02		/* if borrow */
#define M72_CYBW_CYBW		0x03		/* if carry or borrow */

/* M72 read mode flags */
#define M72_READ_LATCH		0x00		/* read counter latch */
#define M72_READ_WAIT		0x01		/* wait for ready irq before read */
#define M72_READ_NOW		0x02		/* force counter latch before read */

/* M72 write mode flags */
#define M72_WRITE_PRELOAD	0x00		/* write preload value */
#define M72_WRITE_NOW		0x02		/* force counter load after write */

/* M72 timer start conditions */
#define M72_TIMER_IN2		0x00		/* at xIN2 rising edge */
#define M72_TIMER_NOW		0x01		/* at now (immediately) */

/* M72 output signal mode (macro) */
/* out: Out_1..4 (1..4) */
/* ch  : Channel (0..3) */
/* mode: Signal Mode (M72_OUT_MODE_xxx) */
#define	M72_OUTCFG(out,ch,mode) \
	(((mode) & 0x03) << (((((out)-1) * 4) + (ch)) * 2) )   

/* M72 output signal mode */
#define M72_OUT_MODE_NO		0x00		/* no action on interrupt */
#define M72_OUT_MODE_HIGH	0x01		/* high signal on interrupt */
#define M72_OUT_MODE_LOW	0x02		/* low signal on interrupt */
#define M72_OUT_MODE_TOGLE	0x03		/* toggle signal on interrupt */

/* M72 output signal setting flags */
#define M72_OUT_SET_CTRL1	0x01		/* Control_1 signal */
#define M72_OUT_SET_CTRL2	0x02		/* Control_2 signal */
#define M72_OUT_SET_CTRL3	0x04		/* Control_3 signal */
#define M72_OUT_SET_CTRL4	0x08		/* Control_4 signal */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
# ifdef MAC_BYTESWAP
	extern void M72_SW_GetEntry(LL_ENTRY* drvP);
	extern void M72_PRE_SW_GetEntry(LL_ENTRY* drvP);
# else
	extern void M72_GetEntry(LL_ENTRY* drvP);
	extern void M72_PRE_GetEntry(LL_ENTRY* drvP);
# endif
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
    /* we have an MDIS4 men_types.h and mdis_api.h included */
    /* only 32bit compatibility needed!                     */
    #define INT32_OR_64  int32
        #define U_INT32_OR_64 u_int32
    typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _M72_DRV_H */




