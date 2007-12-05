/***************************************************************************/
/*                                                                         */
/*  aherrors.h                                                             */
/*                                                                         */
/*    Autohinter error codes (specification only).                         */
/*                                                                         */
/*  Copyright 2001 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This file is used to define the Autohinter error enumeration          */
  /* constants.                                                            */
  /*                                                                       */
  /*************************************************************************/

#ifndef __AHERRORS_H__
#define __AHERRORS_H__

#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#define FT_ERR_PREFIX  AH_Err_
#define FT_ERR_BASE    FT_Mod_Err_Autohint

#include FT_ERRORS_H

#endif /* __AHERRORS_H__ */

/* END */
