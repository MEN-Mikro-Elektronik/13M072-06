#***************************  M a k e f i l e  *******************************
#
#         Author: ls
#          $Date: 2010/04/20 15:01:10 $
#      $Revision: 1.5 $
#
#    Description: Makefile definitions for the M72 driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_sw.mak,v $
#   Revision 1.5  2010/04/20 15:01:10  amorbach
#   R: MDVE test failed
#   M: MAK_SWITCH corrected
#
#   Revision 1.4  2006/10/04 12:12:07  ts
#   changed Standard PLD File back to m72_pld.c
#
#   Revision 1.3  2006/09/27 17:40:31  ts
#   use PLD file Version depending on Variant
#
#   Revision 1.2  2004/08/30 12:19:06  dpfeuffer
#   minor modifications for MDIS4/2004 conformity
#
#   Revision 1.1  2002/03/19 10:54:17  Schoberl
#   Initial Revision
#
#   initial revision (cloned from driver.mak ref 1.3)
#-----------------------------------------------------------------------------
#   (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m72_sw

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED\
		$(SW_PREFIX)MAC_BYTESWAP \
		$(SW_PREFIX)M72_VARIANT=M72_SW \
		$(SW_PREFIX)ID_SW \
		$(SW_PREFIX)PLD_SW


MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id_sw$(LIB_SUFFIX)      \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/pld_sw$(LIB_SUFFIX)      \
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
 
