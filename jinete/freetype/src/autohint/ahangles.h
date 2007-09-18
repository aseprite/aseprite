/***************************************************************************/
/*                                                                         */
/*  ahangles.h                                                             */
/*                                                                         */
/*    A routine used to compute vector angles with limited accuracy        */
/*    and very high speed (specification).                                 */
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


#ifndef __AHANGLES_H__
#define __AHANGLES_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include "ahtypes.h"


FT_BEGIN_HEADER


  /* PI expressed in ah_angles -- we don't really need an important */
  /* precision, so 256 should be enough                             */
#define AH_PI       256
#define AH_2PI      ( AH_PI * 2 )
#define AH_HALF_PI  ( AH_PI / 2 )
#define AH_2PIMASK  ( AH_2PI - 1 )

  /* the number of bits used to express an arc tangent; */
  /* see the structure of the lookup table              */
#define AH_ATAN_BITS  8

  extern
  const AH_Angle  ah_arctan[1L << AH_ATAN_BITS];


  FT_LOCAL AH_Angle
  ah_angle( FT_Vector*  v );


FT_END_HEADER

#endif /* __AHANGLES_H__ */


/* END */
