/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: m72_pld.h
 *
 *       Author: see
 * 
 *  Description: Prototypes for PLD data array and ident function
 *                      
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
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
 
