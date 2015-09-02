#***************************  M a k e f i l e  *******************************
#
#         Author: see
#          $Date: 2004/08/30 12:19:22 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the M72 example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/08/30 12:19:22  dpfeuffer
#   minor modifications for MDIS4/2004 conformity
#
#   Revision 1.1  1998/10/23 16:31:19  see
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m72_freq

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)     \

MAK_INCL=$(MEN_INC_DIR)/m72_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/usr_oss.h     \
         $(MEN_INC_DIR)/usr_utl.h     \

MAK_INP1=m72_freq$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
 
