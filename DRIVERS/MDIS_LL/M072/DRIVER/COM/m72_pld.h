/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: m72_pld.h
 *
 *       Author: see
 *        $Date: 2010/04/20 15:08:20 $
 *    $Revision: 1.3 $
 * 
 *  Description: Prototypes for PLD data array and ident function
 *                      
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m72_pld.h,v $
 * Revision 1.3  2010/04/20 15:08:20  amorbach
 * R: MDVE test failed
 * M: Macros adde to enable variant specific substitution
 *
 * Revision 1.2  2004/08/30 12:18:57  dpfeuffer
 * minor modifications for MDIS4/2004 conformity
 *
 * Revision 1.1  1998/11/02 16:34:08  see
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#ifndef _M72_PLD_H
#define _M72_PLD_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--- macros to make unique names for global symbols ---*/
#ifndef  M72_VARIANT
# define M72_VARIANT M72
#endif

#define _M72_GLOBNAME(var,name) var##_##name

#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define M72_GLOBNAME(var,name) _M72_GLOBNAME(var,name)
#else
# define M72_GLOBNAME(var,name) _M72_GLOBNAME(M72,name)
#endif

#define __M72_PldData			M72_GLOBNAME(M72_VARIANT,PldData)
#define __M72_PldIdent			M72_GLOBNAME(M72_VARIANT,PldIdent)



/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
extern const u_int8 __M72_PldData[];

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
char* __M72_PldIdent(void);


#ifdef __cplusplus
	}
#endif

#endif	/* _M72_PLD_H */
