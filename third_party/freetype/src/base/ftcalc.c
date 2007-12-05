/***************************************************************************/
/*                                                                         */
/*  ftcalc.c                                                               */
/*                                                                         */
/*    Arithmetic computations (body).                                      */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
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
  /* Support for 1-complement arithmetic has been totally dropped in this  */
  /* release.  You can still write your own code if you need it.           */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Implementing basic computation routines.                              */
  /*                                                                       */
  /* FT_MulDiv(), FT_MulFix(), FT_DivFix(), FT_RoundFix(), FT_CeilFix(),   */
  /* and FT_FloorFix() are declared in freetype.h.                         */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H


/* we need to define a 64-bits data type here */
#ifndef FT_CONFIG_OPTION_OLD_CALCS

#ifdef FT_LONG64

  typedef FT_INT64  FT_Int64;

#else

  typedef struct  FT_Int64_
  {
    FT_UInt32  lo;
    FT_UInt32  hi;

  } FT_Int64;

#endif /* FT_LONG64 */

#endif /* !FT_CONFIG_OPTION_OLD_CALCS */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_calc


  /* The following three functions are available regardless of whether */
  /* FT_LONG64 or FT_CONFIG_OPTION_OLD_CALCS is defined.               */

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_RoundFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   ( a + 0x8000L ) & -0x10000L
                      : -((-a + 0x8000L ) & -0x10000L );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_CeilFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   ( a + 0xFFFFL ) & -0x10000L
                      : -((-a + 0xFFFFL ) & -0x10000L );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_FloorFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   a & -0x10000L
                      : -((-a) & -0x10000L );
  }


#ifdef FT_CONFIG_OPTION_OLD_CALCS

  static const FT_Long  ft_square_roots[63] =
  {
       1L,    1L,    2L,     3L,     4L,     5L,     8L,    11L,
      16L,   22L,   32L,    45L,    64L,    90L,   128L,   181L,
     256L,  362L,  512L,   724L,  1024L,  1448L,  2048L,  2896L,
    4096L, 5892L, 8192L, 11585L, 16384L, 23170L, 32768L, 46340L,

      65536L,   92681L,  131072L,   185363L,   262144L,   370727L,
     524288L,  741455L, 1048576L,  1482910L,  2097152L,  2965820L,
    4194304L, 5931641L, 8388608L, 11863283L, 16777216L, 23726566L,

      33554432L,   47453132L,   67108864L,   94906265L,
     134217728L,  189812531L,  268435456L,  379625062L,
     536870912L,  759250125L, 1073741824L, 1518500250L,
    2147483647L
  };

#else

  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )
  FT_Sqrt32( FT_Int32  x )
  {
    FT_ULong  val, root, newroot, mask;


    root = 0;
    mask = 0x40000000L;
    val  = (FT_ULong)x;

    do
    {
      newroot = root + mask;
      if ( newroot <= val )
      {
        val -= newroot;
        root = newroot + mask;
      }

      root >>= 1;
      mask >>= 2;

    } while ( mask != 0 );

    return root;
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#ifdef FT_LONG64

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    FT_Int   s;
    FT_Long  d;


    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }
    if ( c < 0 ) { c = -c; s = -s; }

    d = (FT_Long)( c > 0 ? ( (FT_Int64)a * b + ( c >> 1 ) ) / c
                         : 0x7FFFFFFFL );

    return ( s > 0 ) ? d : -d;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int   s = 1;
    FT_Long  c;


    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }

    c = (FT_Long)( ( (FT_Int64)a * b + 0x8000 ) >> 16 );
    return ( s > 0 ) ? c : -c ;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;

    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }

    if ( b == 0 )
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    else
      /* compute result directly */
      q = (FT_UInt32)( ( ( (FT_Int64)a << 16 ) + ( b >> 1 ) ) / b );

    return ( s < 0 ? -(FT_Long)q : (FT_Long)q );
  }


#ifdef FT_CONFIG_OPTION_OLD_CALCS

  /* a helper function for FT_Sqrt64() */

  static int
  ft_order64( FT_Int64  z )
  {
    int  j = 0;


    while ( z )
    {
      z = (unsigned FT_INT64)z >> 1;
      j++;
    }
    return j - 1;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )
  FT_Sqrt64( FT_Int64  l )
  {
    FT_Int64  r, s;


    if ( l <= 0 ) return 0;
    if ( l == 1 ) return 1;

    r = ft_square_roots[ft_order64( l )];

    do
    {
      s = r;
      r = ( r + l / r ) >> 1;

    } while ( r > s || r * r > l );

    return (FT_Int32)r;
  }

#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#else /* FT_LONG64 */


  static void
  ft_multo64( FT_UInt32  x,
              FT_UInt32  y,
              FT_Int64  *z )
  {
    FT_UInt32  lo1, hi1, lo2, hi2, lo, hi, i1, i2;


    lo1 = x & 0x0000FFFFU;  hi1 = x >> 16;
    lo2 = y & 0x0000FFFFU;  hi2 = y >> 16;

    lo = lo1 * lo2;
    i1 = lo1 * hi2;
    i2 = lo2 * hi1;
    hi = hi1 * hi2;

    /* Check carry overflow of i1 + i2 */
    i1 += i2;
    hi += (FT_UInt32)( i1 < i2 ) << 16;

    hi += i1 >> 16;
    i1  = i1 << 16;

    /* Check carry overflow of i1 + lo */
    lo += i1;
    hi += ( lo < i1 );

    z->lo = lo;
    z->hi = hi;
  }


  static FT_UInt32
  ft_div64by32( FT_UInt32  hi,
                FT_UInt32  lo,
                FT_UInt32  y )
  {
    FT_UInt32  r, q;
    FT_Int     i;


    q = 0;
    r = hi;

    if ( r >= y )
      return (FT_UInt32)0x7FFFFFFFL;

    i = 32;
    do
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (FT_UInt32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    } while ( --i );

    return q;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( void )
  FT_Add64( FT_Int64*  x,
            FT_Int64*  y,
            FT_Int64  *z )
  {
    register FT_UInt32  lo, hi, max;


    max = x->lo > y->lo ? x->lo : y->lo;
    lo  = x->lo + y->lo;
    hi  = x->hi + y->hi + ( lo < max );

    z->lo = lo;
    z->hi = hi;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    long  s;


    if ( a == 0 || b == c )
      return a;

    s  = a; a = ABS( a );
    s ^= b; b = ABS( b );
    s ^= c; c = ABS( c );

    if ( a <= 46340L && b <= 46340L && c <= 176095L && c > 0 )
    {
      a = ( a * b + ( c >> 1 ) ) / c;
    }
    else if ( c > 0 )
    {
      FT_Int64  temp, temp2;


      ft_multo64( a, b, &temp );

      temp2.hi = 0;
      temp2.lo = (FT_UInt32)(c >> 1);
      FT_Add64( &temp, &temp2, &temp );
      a = ft_div64by32( temp.hi, temp.lo, c );
    }
    else
      a = 0x7FFFFFFFL;

    return ( s < 0 ? -a : a );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Long   s;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
    {
      ua = ( ua * ub + 0x8000 ) >> 16;
    }
    else
    {
      FT_ULong  al = ua & 0xFFFF;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFF ) + 0x8000 ) >> 16 );
    }

    return ( s < 0 ? -(FT_Long)ua : ua );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = a; a = ABS(a);
    s ^= b; b = ABS(b);

    if ( b == 0 )
    {
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    }
    else if ( ( a >> 16 ) == 0 )
    {
      /* compute result directly */
      q = (FT_UInt32)( (a << 16) + (b >> 1) ) / (FT_UInt32)b;
    }
    else
    {
      /* we need more bits; we have to do it by hand */
      FT_Int64  temp, temp2;

      temp.hi  = (FT_Int32) (a >> 16);
      temp.lo  = (FT_UInt32)(a << 16);
      temp2.hi = 0;
      temp2.lo = (FT_UInt32)( b >> 1 );
      FT_Add64( &temp, &temp2, &temp );
      q = ft_div64by32( temp.hi, temp.lo, b );
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( void )
  FT_MulTo64( FT_Int32   x,
              FT_Int32   y,
              FT_Int64  *z )
  {
    FT_Int32  s;


    s  = x; x = ABS( x );
    s ^= y; y = ABS( y );

    ft_multo64( x, y, z );

    if ( s < 0 )
    {
      z->lo = (FT_UInt32)-(FT_Int32)z->lo;
      z->hi = ~z->hi + !( z->lo );
    }
  }


  /* documentation is in ftcalc.h */

  /* apparently, the second version of this code is not compiled correctly */
  /* on Mac machines with the MPW C compiler..  tsss, tsss, tss...         */
#if 1
  FT_EXPORT_DEF( FT_Int32 )
  FT_Div64by32( FT_Int64*  x,
                FT_Int32   y )
  {
    FT_Int32   s;
    FT_UInt32  q, r, i, lo;


    s  = x->hi;
    if ( s < 0 )
    {
      x->lo = (FT_UInt32)-(FT_Int32)x->lo;
      x->hi = ~x->hi + !x->lo;
    }
    s ^= y;  y = ABS( y );

    /* Shortcut */
    if ( x->hi == 0 )
    {
      if ( y > 0 )
        q = x->lo / y;
      else
        q = 0x7FFFFFFFL;

      return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
    }

    r  = x->hi;
    lo = x->lo;

    if ( r >= (FT_UInt32)y ) /* we know y is to be treated as unsigned here */
      return ( s < 0 ? 0x80000001UL : 0x7FFFFFFFUL );
                             /* Return Max/Min Int32 if division overflow. */
                             /* This includes division by zero! */
    q = 0;
    for ( i = 0; i < 32; i++ )
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (FT_UInt32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }
#else
  FT_EXPORT_DEF( FT_Int32 )
  FT_Div64by32( FT_Int64*  x,
                FT_Int32   y )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = x->hi;
    if ( s < 0 )
    {
      x->lo = (FT_UInt32)-(FT_Int32)x->lo;
      x->hi = ~x->hi + !x->lo;
    }
    s ^= y;  y = ABS( y );

    /* Shortcut */
    if ( x->hi == 0 )
    {
      if ( y > 0 )
        q = ( x->lo + ( y >> 1 ) ) / y;
      else
        q = 0x7FFFFFFFL;

      return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
    }

    q = ft_div64by32( x->hi, x->lo, y );

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }
#endif


#ifdef FT_CONFIG_OPTION_OLD_CALCS


  /* two helper functions for FT_Sqrt64() */

  static void
  FT_Sub64( FT_Int64*  x,
            FT_Int64*  y,
            FT_Int64*  z )
  {
    register FT_UInt32  lo, hi;


    lo = x->lo - y->lo;
    hi = x->hi - y->hi - ( (FT_Int32)lo < 0 );

    z->lo = lo;
    z->hi = hi;
  }


  static int
  ft_order64( FT_Int64*  z )
  {
    FT_UInt32  i;
    int        j;


    i = z->lo;
    j = 0;
    if ( z->hi )
    {
      i = z->hi;
      j = 32;
    }

    while ( i > 0 )
    {
      i >>= 1;
      j++;
    }
    return j - 1;
  }


  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )
  FT_Sqrt64( FT_Int64*  l )
  {
    FT_Int64  l2;
    FT_Int32  r, s;


    if ( (FT_Int32)l->hi < 0          ||
         ( l->hi == 0 && l->lo == 0 ) )
      return 0;

    s = ft_order64( l );
    if ( s == 0 )
      return 1;

    r = ft_square_roots[s];
    do
    {
      s = r;
      r = ( r + FT_Div64by32( l, r ) ) >> 1;
      FT_MulTo64( r, r,   &l2 );
      FT_Sub64  ( l, &l2, &l2 );

    } while ( r > s || (FT_Int32)l2.hi < 0 );

    return r;
  }


#endif /* FT_CONFIG_OPTION_OLD_CALCS */


#endif /* FT_LONG64 */


  /* a not-so-fast but working 16.16 fixed point square root function */

  FT_EXPORT_DEF( FT_Int32 )
  FT_SqrtFixed( FT_Int32  x )
  {
    FT_UInt32  root, rem_hi, rem_lo, test_div;
    FT_Int     count;


    root = 0;

    if ( x > 0 )
    {
      rem_hi = 0;
      rem_lo = x;
      count  = 24;
      do
      {
        rem_hi   = ( rem_hi << 2 ) | ( rem_lo >> 30 );
        rem_lo <<= 2;
        root   <<= 1;
        test_div = ( root << 1 ) + 1;

        if ( rem_hi >= test_div )
        {
          rem_hi -= test_div;
          root   += 1;
        }
      } while ( --count );
    }

    return (FT_Int32)root;
  }


/* END */
