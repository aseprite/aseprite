/***************************************************************************/
/*                                                                         */
/*  ahangles.h                                                             */
/*                                                                         */
/*    A routine used to compute vector angles with limited accuracy        */
/*    and very high speed (body).                                          */
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


#include <ft2build.h>
#include "ahangles.h"


  /* the following table has been automatically generated with */
  /* the `mather.py' Python script                             */

  const AH_Angle  ah_arctan[1L << AH_ATAN_BITS] =
  {
     0,  0,  1,  1,  1,  2,  2,  2,
     3,  3,  3,  3,  4,  4,  4,  5,
     5,  5,  6,  6,  6,  7,  7,  7,
     8,  8,  8,  9,  9,  9, 10, 10,
    10, 10, 11, 11, 11, 12, 12, 12,
    13, 13, 13, 14, 14, 14, 14, 15,
    15, 15, 16, 16, 16, 17, 17, 17,
    18, 18, 18, 18, 19, 19, 19, 20,
    20, 20, 21, 21, 21, 21, 22, 22,
    22, 23, 23, 23, 24, 24, 24, 24,
    25, 25, 25, 26, 26, 26, 26, 27,
    27, 27, 28, 28, 28, 28, 29, 29,
    29, 30, 30, 30, 30, 31, 31, 31,
    31, 32, 32, 32, 33, 33, 33, 33,
    34, 34, 34, 34, 35, 35, 35, 35,
    36, 36, 36, 36, 37, 37, 37, 38,
    38, 38, 38, 39, 39, 39, 39, 40,
    40, 40, 40, 41, 41, 41, 41, 42,
    42, 42, 42, 42, 43, 43, 43, 43,
    44, 44, 44, 44, 45, 45, 45, 45,
    46, 46, 46, 46, 46, 47, 47, 47,
    47, 48, 48, 48, 48, 48, 49, 49,
    49, 49, 50, 50, 50, 50, 50, 51,
    51, 51, 51, 51, 52, 52, 52, 52,
    52, 53, 53, 53, 53, 53, 54, 54,
    54, 54, 54, 55, 55, 55, 55, 55,
    56, 56, 56, 56, 56, 57, 57, 57,
    57, 57, 57, 58, 58, 58, 58, 58,
    59, 59, 59, 59, 59, 59, 60, 60,
    60, 60, 60, 61, 61, 61, 61, 61,
    61, 62, 62, 62, 62, 62, 62, 63,
    63, 63, 63, 63, 63, 64, 64, 64
  };


  FT_LOCAL_DEF AH_Angle
  ah_angle( FT_Vector*  v )
  {
    FT_Pos    dx, dy;
    AH_Angle  angle;


    dx = v->x;
    dy = v->y;

    /* check trivial cases */
    if ( dy == 0 )
    {
      angle = 0;
      if ( dx < 0 )
        angle = AH_PI;
      return angle;
    }
    else if ( dx == 0 )
    {
      angle = AH_HALF_PI;
      if ( dy < 0 )
        angle = -AH_HALF_PI;
      return angle;
    }

    angle = 0;
    if ( dx < 0 )
    {
      dx = -v->x;
      dy = -v->y;
      angle = AH_PI;
    }

    if ( dy < 0 )
    {
      FT_Pos  tmp;


      tmp = dx;
      dx  = -dy;
      dy  = tmp;
      angle -= AH_HALF_PI;
    }

    if ( dx == 0 && dy == 0 )
      return 0;

    if ( dx == dy )
      angle += AH_PI / 4;
    else if ( dx > dy )
      angle += ah_arctan[FT_DivFix( dy, dx ) >> ( 16 - AH_ATAN_BITS )];
    else
      angle += AH_HALF_PI -
               ah_arctan[FT_DivFix( dx, dy ) >> ( 16 - AH_ATAN_BITS )];

    if ( angle > AH_PI )
      angle -= AH_2PI;

    return angle;
  }


/* END */
