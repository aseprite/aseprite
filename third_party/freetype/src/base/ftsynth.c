/***************************************************************************/
/*                                                                         */
/*  ftsynth.c                                                              */
/*                                                                         */
/*    FreeType synthesizing code for emboldening and slanting (body).      */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_CALC_H
#include FT_OUTLINE_H
#include FT_SYNTHESIS_H


#define FT_BOLD_THRESHOLD  0x0100


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL OBLIQUING SUPPORT                                ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/

  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Oblique( FT_GlyphSlot  original,
                      FT_Outline*   outline,
                      FT_Pos*       advance )
  {
    FT_Matrix  transform;

    FT_UNUSED( original );
    /* we don't touch the advance width */
    FT_UNUSED( advance );



    /* For italic, simply apply a shear transform, with an angle */
    /* of about 12 degrees.                                      */

    transform.xx = 0x10000L;
    transform.yx = 0x00000L;

    transform.xy = 0x06000L;
    transform.yy = 0x10000L;

    FT_Outline_Transform( outline, &transform );

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   EXPERIMENTAL EMBOLDENING/OUTLINING SUPPORT                    ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /* Compute the norm of a vector */

#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static FT_Pos
  ft_norm( FT_Vector*  vec )
  {
    FT_Int64  t1, t2;


    MUL_64( vec->x, vec->x, t1 );
    MUL_64( vec->y, vec->y, t2 );
    ADD_64( t1, t2, t1 );

    return (FT_Pos)SQRT_64( t1 );
  }

#else /* FT_CONFIG_OPTION_OLD_CALCS */

  static FT_Pos
  ft_norm( FT_Vector*  vec )
  {
    FT_F26Dot6  u, v, d;
    FT_Int      shift;
    FT_ULong    H, L, L2, hi, lo, med;


    u = vec->x; if ( u < 0 ) u = -u;
    v = vec->y; if ( v < 0 ) v = -v;

    if ( u < v )
    {
      d = u;
      u = v;
      v = d;
    }

    /* check that we are not trying to normalize zero! */
    if ( u == 0 )
      return 0;

    /* compute (u*u + v*v) on 64 bits with two 32-bit registers [H:L] */
    hi  = (FT_ULong)u >> 16;
    lo  = (FT_ULong)u & 0xFFFF;
    med = hi * lo;

    H     = hi * hi + ( med >> 15 );
    med <<= 17;
    L     = lo * lo + med;
    if ( L < med )
      H++;

    hi  = (FT_ULong)v >> 16;
    lo  = (FT_ULong)v & 0xFFFF;
    med = hi * lo;

    H    += hi * hi + ( med >> 15 );
    med <<= 17;
    L2    = lo * lo + med;
    if ( L2 < med )
      H++;

    L += L2;
    if ( L < L2 )
      H++;

    /* if the value is smaller than 32 bits */
    shift = 0;
    if ( H == 0 )
    {
      while ( ( L & 0xC0000000UL ) == 0 )
      {
        L <<= 2;
        shift++;
      }
      return ( FT_Sqrt32( L ) >> shift );
    }
    else
    {
      while ( H )
      {
        L   = ( L >> 2 ) | ( H << 30 );
        H >>= 2;
        shift++;
      }
      return ( FT_Sqrt32( L ) << shift );
    }
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


  static int
  ft_test_extrema( FT_Outline*  outline,
                   int          n )
  {
    FT_Vector  *prev, *cur, *next;
    FT_Pos      product;
    FT_Int      c, first, last;


    /* we need to compute the `previous' and `next' point */
    /* for these extrema.                                 */
    cur   = outline->points + n;
    prev  = cur - 1;
    next  = cur + 1;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      last  = outline->contours[c];

      if ( n == first )
        prev = outline->points + last;

      if ( n == last )
        next = outline->points + first;

      first = last + 1;
    }

    product = FT_MulDiv( cur->x - prev->x,   /* in.x  */
                         next->y - cur->y,   /* out.y */
                         0x40 )
              -
              FT_MulDiv( cur->y - prev->y,   /* in.y  */
                         next->x - cur->x,   /* out.x */
                         0x40 );

    if ( product )
      product = product > 0 ? 1 : -1;

    return product;
  }


  /* Compute the orientation of path filling.  It differs between TrueType */
  /* and Type1 formats.  We could use the `ft_outline_reverse_fill' flag,  */
  /* but it is better to re-compute it directly (it seems that this flag   */
  /* isn't correctly set for some weird composite glyphs currently).       */
  /*                                                                       */
  /* We do this by computing bounding box points, and computing their      */
  /* curvature.                                                            */
  /*                                                                       */
  /* The function returns either 1 or -1.                                  */
  /*                                                                       */
  static int
  ft_get_orientation( FT_Outline*  outline )
  {
    FT_BBox  box;
    FT_BBox  indices;
    int      n, last;


    indices.xMin = -1;
    indices.yMin = -1;
    indices.xMax = -1;
    indices.yMax = -1;

    box.xMin = box.yMin =  32767;
    box.xMax = box.yMax = -32768;

    /* is it empty ? */
    if ( outline->n_contours < 1 )
      return 1;

    last = outline->contours[outline->n_contours - 1];

    for ( n = 0; n <= last; n++ )
    {
      FT_Pos  x, y;


      x = outline->points[n].x;
      if ( x < box.xMin )
      {
        box.xMin     = x;
        indices.xMin = n;
      }
      if ( x > box.xMax )
      {
        box.xMax     = x;
        indices.xMax = n;
      }

      y = outline->points[n].y;
      if ( y < box.yMin )
      {
        box.yMin     = y;
        indices.yMin = n;
      }
      if ( y > box.yMax )
      {
        box.yMax     = y;
        indices.yMax = n;
      }
    }

    /* test orientation of the xmin */
    n = ft_test_extrema( outline, indices.xMin );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.yMin );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.xMax );
    if ( n )
      goto Exit;

    n = ft_test_extrema( outline, indices.yMax );
    if ( !n )
      n = 1;

  Exit:
    return n;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_Outline_Embolden( FT_GlyphSlot original,
                       FT_Outline*  outline,
                       FT_Pos*      advance )
  {
    FT_Vector   u, v;
    FT_Vector*  points;
    FT_Vector   cur, prev, next;
    FT_Pos      distance;
    FT_Face     face = FT_SLOT_FACE( original );
    int         c, n, first, orientation;

    FT_UNUSED( advance );


    /* compute control distance */
    distance = FT_MulFix( face->units_per_EM / 60,
                          face->size->metrics.y_scale );

    orientation = ft_get_orientation( &original->outline );

    points = original->outline.points;

    first = 0;
    for ( c = 0; c < outline->n_contours; c++ )
    {
      int  last = outline->contours[c];


      prev = points[last];

      for ( n = first; n <= last; n++ )
      {
        FT_Pos     norm, delta, d;
        FT_Vector  in, out;


        cur = points[n];
        if ( n < last ) next = points[n + 1];
        else            next = points[first];

        /* compute the in and out vectors */
        in.x  = cur.x - prev.x;
        in.y  = cur.y - prev.y;

        out.x = next.x - cur.x;
        out.y = next.y - cur.y;

        /* compute U and V */
        norm = ft_norm( &in );
        u.x = orientation *  FT_DivFix( in.y, norm );
        u.y = orientation * -FT_DivFix( in.x, norm );

        norm = ft_norm( &out );
        v.x = orientation *  FT_DivFix( out.y, norm );
        v.y = orientation * -FT_DivFix( out.x, norm );

        d = distance;

        if ( ( outline->tags[n] & FT_Curve_Tag_On ) == 0 )
          d *= 2;

        /* Check discriminant for parallel vectors */
        delta = FT_MulFix( u.x, v.y ) - FT_MulFix( u.y, v.x );
        if ( delta > FT_BOLD_THRESHOLD || delta < -FT_BOLD_THRESHOLD )
        {
          /* Move point -- compute A and B */
          FT_Pos  x, y, A, B;


          A = d + FT_MulFix( cur.x, u.x ) + FT_MulFix( cur.y, u.y );
          B = d + FT_MulFix( cur.x, v.x ) + FT_MulFix( cur.y, v.y );

          x = FT_MulFix( A, v.y ) - FT_MulFix( B, u.y );
          y = FT_MulFix( B, u.x ) - FT_MulFix( A, v.x );

          outline->points[n].x = distance + FT_DivFix( x, delta );
          outline->points[n].y = distance + FT_DivFix( y, delta );
        }
        else
        {
          /* Vectors are nearly parallel */
          FT_Pos  x, y;


          x = distance + cur.x + FT_MulFix( d, u.x + v.x ) / 2;
          y = distance + cur.y + FT_MulFix( d, u.y + v.y ) / 2;

          outline->points[n].x = x;
          outline->points[n].y = y;
        }

        prev = cur;
      }

      first = last + 1;
    }

    if ( advance )
      *advance = ( *advance + distance * 4 ) & -64;

    return 0;
  }


/* END */
