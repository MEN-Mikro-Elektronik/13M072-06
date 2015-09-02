#***************************  M a k e f i l e  *******************************
#
#         Author: see
#          $Date: 2004/08/30 12:19:17 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for M72 tools
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/08/30 12:19:17  dpfeuffer
#   minor modifications for MDIS4/2004 conformity
#
#   Revision 1.1  1998/10/23 16:31:08  see
#   Added by mcvs
#
#   Revision 1.1  1998/10/01 15:54:43  see
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m72_count

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         
MAK_INCL=$(MEN_INC_DIR)/m72_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/usr_oss.h     \
         $(MEN_INC_DIR)/usr_utl.h     \

MAK_INP1=m72_count$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

 
