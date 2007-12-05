/***************************************************************************/
/*                                                                         */
/*  fttrigon.c                                                             */
/*                                                                         */
/*    FreeType trigonometric functions (body).                             */
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


#include <ft2build.h>
#include FT_TRIGONOMETRY_H


  /* the following is 0.2715717684432231 * 2^30 */
#define FT_TRIG_COSCALE  0x11616E8EUL

  /* this table was generated for FT_PI = 180L << 16, i.e. degrees */
#define FT_TRIG_MAX_ITERS  23

  static const FT_Fixed
  ft_trig_arctan_table[24] =
  {
    4157273L, 2949120L, 1740967L, 919879L, 466945L, 234379L, 117304L,
    58666L, 29335L, 14668L, 7334L, 3667L, 1833L, 917L, 458L, 229L, 115L,
    57L, 29L, 14L, 7L, 4L, 2L, 1L
  };

  /* the Cordic shrink factor, multiplied by 2^32 */
#define FT_TRIG_SCALE    1166391785UL  /* 0x4585BA38UL */


#ifdef FT_CONFIG_HAS_INT64

  /* multiply a given value by the CORDIC shrink factor */
  static FT_Fixed
  ft_trig_downscale( FT_Fixed  val )
  {
    FT_Fixed  s;
    FT_Int64  v;


    s   = val;
    val = ( val >= 0 ) ? val : -val;

    v   = ( val * (FT_Int64)FT_TRIG_SCALE ) + 0x100000000UL;
    val = (FT_Fixed)( v >> 32 );

    return ( s >= 0 ) ? val : -val;
  }

#else /* !FT_CONFIG_HAS_INT64 */

  /* multiply a given value by the CORDIC shrink factor */
  static FT_Fixed
  ft_trig_downscale( FT_Fixed  val )
  {
    FT_Fixed   s;
    FT_UInt32  v1, v2, k1, k2, hi, lo1, lo2, lo3;


    s   = val;
    val = ( val >= 0 ) ? val : -val;

    v1 = (FT_UInt32)val >> 16;
    v2 = (FT_UInt32)val & 0xFFFF;

    k1 = FT_TRIG_SCALE >> 16;       /* constant */
    k2 = FT_TRIG_SCALE & 0xFFFF;    /* constant */

    hi   = k1 * v1;
    lo1  = k1 * v2 + k2 * v1;       /* can't overflow */

    lo2  = ( k2 * v2 ) >> 16;
    lo3  = ( lo1 >= lo2 ) ? lo1 : lo2;
    lo1 += lo2;

    hi  += lo1 >> 16;
    if ( lo1 < lo3 )
      hi += 0x10000UL;

    val  = (FT_Fixed)hi;

    return ( s >= 0 ) ? val : -val;
  }

#endif /* !FT_CONFIG_HAS_INT64 */


  static FT_Int
  ft_trig_prenorm( FT_Vector*  vec )
  {
    FT_Fixed  x, y, z;
    FT_Int    shift;


    x = vec->x;
    y = vec->y;

    z     = ( ( x >= 0 ) ? x : - x ) | ( (y >= 0) ? y : -y );
    shift = 0;

    if ( z < ( 1L << 27 ) )
    {
      do
      {
        shift++;
        z <<= 1;
      } while ( z < ( 1L << 27 ) );

      vec->x = x << shift;
      vec->y = y << shift;
    }
    else if ( z > ( 1L << 28 ) )
    {
      do
      {
        shift++;
        z >>= 1;
      } while ( z > ( 1L << 28 ) );

      vec->x = x >> shift;
      vec->y = y >> shift;
      shift  = -shift;
    }
    return shift;
  }


  static void
  ft_trig_pseudo_rotate( FT_Vector*  vec,
                         FT_Angle    theta )
  {
    FT_Int           i;
    FT_Fixed         x, y, xtemp;
    const FT_Fixed  *arctanptr;


    x = vec->x;
    y = vec->y;

    /* Get angle between -90 and 90 degrees */
    while ( theta <= -FT_ANGLE_PI2 )
    {
      x = -x;
      y = -y;
      theta += FT_ANGLE_PI;
    }

    while ( theta > FT_ANGLE_PI2 )
    {
      x = -x;
      y = -y;
      theta -= FT_ANGLE_PI;
    }

    /* Initial pseudorotation, with left shift */
    arctanptr = ft_trig_arctan_table;

    if ( theta < 0 )
    {
      xtemp  = x + ( y << 1 );
      y      = y - ( x << 1 );
      x      = xtemp;
      theta += *arctanptr++;
    }
    else
    {
      xtemp  = x - ( y << 1 );
      y      = y + ( x << 1 );
      x      = xtemp;
      theta -= *arctanptr++;
    }

    /* Subsequent pseudorotations, with right shifts */
    i = 0;
    do
    {
      if ( theta < 0 )
      {
        xtemp  = x + ( y >> i );
        y      = y - ( x >> i );
        x      = xtemp;
        theta += *arctanptr++;
      }
      else
      {
        xtemp  = x - ( y >> i );
        y      = y + ( x >> i );
        x      = xtemp;
        theta -= *arctanptr++;
      }
    } while ( ++i < FT_TRIG_MAX_ITERS );

    vec->x = x;
    vec->y = y;
  }


  static void
  ft_trig_pseudo_polarize( FT_Vector*  vec )
  {
    FT_Fixed         theta;
    FT_Fixed         yi, i;
    FT_Fixed         x, y;
    const FT_Fixed  *arctanptr;


    x = vec->x;
    y = vec->y;

    /* Get the vector into the right half plane */
    theta = 0;
    if ( x < 0 )
    {
      x = -x;
      y = -y;
      theta = 2 * FT_ANGLE_PI2;
    }

    if ( y > 0 )
      theta = - theta;

    arctanptr = ft_trig_arctan_table;

    if ( y < 0 )
    {
      /* Rotate positive */
      yi     = y + ( x << 1 );
      x      = x - ( y << 1 );
      y      = yi;
      theta -= *arctanptr++;  /* Subtract angle */
    }
    else
    {
      /* Rotate negative */
      yi     = y - ( x << 1 );
      x      = x + ( y << 1 );
      y      = yi;
      theta += *arctanptr++;  /* Add angle */
    }

    i = 0;
    do
    {
      if ( y < 0 )
      {
        /* Rotate positive */
        yi     = y + ( x >> i );
        x      = x - ( y >> i );
        y      = yi;
        theta -= *arctanptr++;
      }
      else
      {
        /* Rotate negative */
        yi     = y - ( x >> i );
        x      = x + ( y >> i );
        y      = yi;
        theta += *arctanptr++;
      }
    } while ( ++i < FT_TRIG_MAX_ITERS );

    /* round theta */
    if ( theta >= 0 )
      theta = ( theta + 16 ) & -32;
    else
      theta = - (( -theta + 16 ) & -32);

    vec->x = x;
    vec->y = theta;
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_Cos( FT_Angle  angle )
  {
    FT_Vector  v;


    v.x = FT_TRIG_COSCALE >> 2;
    v.y = 0;
    ft_trig_pseudo_rotate( &v, angle );

    return v.x / ( 1 << 12 );
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_Sin( FT_Angle  angle )
  {
    return FT_Cos( FT_ANGLE_PI2 - angle );
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_Tan( FT_Angle  angle )
  {
    FT_Vector  v;


    v.x = FT_TRIG_COSCALE >> 2;
    v.y = 0;
    ft_trig_pseudo_rotate( &v, angle );

    return FT_DivFix( v.y, v.x );
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( FT_Angle )
  FT_Atan2( FT_Fixed  dx,
            FT_Fixed  dy )
  {
    FT_Vector  v;


    if ( dx == 0 && dy == 0 )
      return 0;

    v.x = dx;
    v.y = dy;
    ft_trig_prenorm( &v );
    ft_trig_pseudo_polarize( &v );

    return v.y;
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( void )
  FT_Vector_Unit( FT_Vector*  vec,
                  FT_Angle    angle )
  {
    vec->x = FT_TRIG_COSCALE >> 2;
    vec->y = 0;
    ft_trig_pseudo_rotate( vec, angle );
    vec->x >>= 12;
    vec->y >>= 12;
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( void )
  FT_Vector_Rotate( FT_Vector*  vec,
                    FT_Angle    angle )
  {
    FT_Int     shift;
    FT_Vector  v;


    v.x   = vec->x;
    v.y   = vec->y;

    if ( angle && ( v.x != 0 || v.y != 0 ) )
    {
      shift = ft_trig_prenorm( &v );
      ft_trig_pseudo_rotate( &v, angle );
      v.x = ft_trig_downscale( v.x );
      v.y = ft_trig_downscale( v.y );

      if ( shift >= 0 )
      {
        vec->x = v.x >> shift;
        vec->y = v.y >> shift;
      }
      else
      {
        shift  = -shift;
        vec->x = v.x << shift;
        vec->y = v.y << shift;
      }
    }
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_Vector_Length( FT_Vector*  vec )
  {
    FT_Int     shift;
    FT_Vector  v;


    v = *vec;

    /* handle trivial cases */
    if ( v.x == 0 )
    {
      return ( v.y >= 0 ) ? v.y : -v.y;
    }
    else if ( v.y == 0 )
    {
      return ( v.x >= 0 ) ? v.x : -v.x;
    }

    /* general case */
    shift = ft_trig_prenorm( &v );
    ft_trig_pseudo_polarize( &v );

    v.x = ft_trig_downscale( v.x );
    return ( shift >= 0 ) ? ( v.x >> shift ) : ( v.x << -shift );
  }


  /* documentation is in fttrigon.h */

  FT_EXPORT_DEF( void )
  FT_Vector_Polarize( FT_Vector*  vec,
                      FT_Fixed   *length,
                      FT_Angle   *angle )
  {
    FT_Int     shift;
    FT_Vector  v;


    v = *vec;

    if ( v.x == 0 && v.y == 0 )
      return;

    shift = ft_trig_prenorm( &v );
    ft_trig_pseudo_polarize( &v );

    v.x = ft_trig_downscale( v.x );

    *length = ( shift >= 0 ) ? ( v.x >> shift ) : ( v.x << -shift );
    *angle  = v.y;
  }


/* END */
