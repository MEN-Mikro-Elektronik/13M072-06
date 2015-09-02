#***************************  M a k e f i l e  *******************************
#
#         Author: see
#          $Date: 2010/04/20 14:59:59 $
#      $Revision: 1.8 $
#
#    Description: Makefile definitions for the M72 driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#   Revision 1.8  2010/04/20 14:59:59  amorbach
#   R: MDVE test failed
#   M: M72_VARIANT added
#
#   Revision 1.7  2006/10/04 12:11:58  ts
#   changed Standard PLD File back to m72_pld.c
#
#   Revision 1.6  2006/09/26 18:48:32  ts
#   use PLD file Version depending on Variant
#
#   Revision 1.5  2004/08/30 12:18:59  dpfeuffer
#   minor modifications for MDIS4/2004 conformity
#
#   Revision 1.4  2002/03/19 10:54:14  Schoberl
#   added MAK_SWITCH MAC_MEM_MAPPED
#
#   Revision 1.3  1999/08/06 09:18:59  Schoberl
#   microwire added
#
#   Revision 1.2  1998/11/04 14:50:21  Schmidt
#   mbuf removed
#
#   Revision 1.1  1998/10/23 16:30:45  see
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m72

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
		$(SW_PREFIX)M72_VARIANT=M72

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/pld$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)     \


MAK_INCL=$(MEN_MOD_DIR)/m72_pld.h     \
		 $(MEN_INC_DIR)/m72_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/maccess.h     \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/pld_load.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_com.h    \
         $(MEN_INC_DIR)/microwire.h      \
         $(MEN_INC_DIR)/ll_defs.h     \
         $(MEN_INC_DIR)/ll_entry.h    \
         $(MEN_INC_DIR)/dbg.h         \

MAK_INP1=m72_drv$(INP_SUFFIX)
MAK_INP2=m72_pld$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2)

 
