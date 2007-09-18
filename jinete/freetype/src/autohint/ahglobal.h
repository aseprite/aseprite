/***************************************************************************/
/*                                                                         */
/*  ahglobal.h                                                             */
/*                                                                         */
/*    Routines used to compute global metrics automatically                */
/*    (specification).                                                     */
/*                                                                         */
/*  Copyright 2000-2001 Catharon Productions Inc.                          */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#ifndef __AHGLOBAL_H__
#define __AHGLOBAL_H__


#include <ft2build.h>
#include "ahtypes.h"
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


#define AH_IS_TOP_BLUE( b )  ( (b) == ah_blue_capital_top || \
                               (b) == ah_blue_small_top   )


  /* compute global metrics automatically */
  FT_LOCAL
  FT_Error  ah_hinter_compute_globals( AH_Hinter*  hinter );


FT_END_HEADER

#endif /* __AHGLOBAL_H__ */


/* END */
