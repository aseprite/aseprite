/***************************************************************************/
/*                                                                         */
/*  ftcalc.h                                                               */
/*                                                                         */
/*    Arithmetic computations (specification).                             */
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


#ifndef __FTCALC_H__
#define __FTCALC_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


/* OLD 64-bits internal API */

#ifdef FT_CONFIG_OPTION_OLD_CALCS

#ifdef FT_LONG64

  typedef FT_INT64  FT_Int64;

#define ADD_64( x, y, z )  z = (x) + (y)
#define MUL_64( x, y, z )  z = (FT_Int64)(x) * (y)
#define DIV_64( x, y )     ( (x) / (y) )

#define SQRT_64( z )  FT_Sqrt64( z )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Sqrt64                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the square root of a 64-bit value.  That sounds stupid,   */
  /*    but it is needed to obtain maximal accuracy in the TrueType        */
  /*    bytecode interpreter.                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    l :: A 64-bit integer.                                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The 32-bit square-root.                                            */
  /*                                                                       */
  FT_EXPORT( FT_Int32 )
  FT_Sqrt64( FT_Int64  l );


#else /* !FT_LONG64 */


  typedef struct  FT_Int64_
  {
    FT_UInt32  lo;
    FT_UInt32  hi;

  } FT_Int64;


#define ADD_64( x, y, z )  FT_Add64( &x, &y, &z )
#define MUL_64( x, y, z )  FT_MulTo64( x, y, &z )
#define DIV_64( x, y )     FT_Div64by32( &x, y )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add64                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Add two Int64 values.                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: A pointer to the first value to be added.                     */
  /*    y :: A pointer to the second value to be added.                    */
  /*                                                                       */
  /* <Output>                                                              */
  /*    z :: A pointer to the result of `x + y'.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Will be wrapped by the ADD_64() macro.                             */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Add64( FT_Int64*  x,
            FT_Int64*  y,
            FT_Int64  *z );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulTo64                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Multiplies two Int32 integers.  Returns an Int64 integer.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: The first multiplier.                                         */
  /*    y :: The second multiplier.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    z :: A pointer to the result of `x * y'.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Will be wrapped by the MUL_64() macro.                             */
  /*                                                                       */
  FT_EXPORT( void )
  FT_MulTo64( FT_Int32   x,
              FT_Int32   y,
              FT_Int64  *z );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Div64by32                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Divides an Int64 value by an Int32 value.  Returns an Int32        */
  /*    integer.                                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: A pointer to the dividend.                                    */
  /*    y :: The divisor.                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `x / y'.                                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Will be wrapped by the DIV_64() macro.                             */
  /*                                                                       */
  FT_EXPORT( FT_Int32 )
  FT_Div64by32( FT_Int64*  x,
                FT_Int32   y );


#define SQRT_64( z )  FT_Sqrt64( &z )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Sqrt64                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the square root of a 64-bits value.  That sounds stupid,  */
  /*    but it is needed to obtain maximal accuracy in the TrueType        */
  /*    bytecode interpreter.                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    z :: A pointer to a 64-bit integer.                                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The 32-bit square-root.                                            */
  /*                                                                       */
  FT_EXPORT( FT_Int32 )
  FT_Sqrt64( FT_Int64*  x );


#endif /* !FT_LONG64 */

#endif /* FT_CONFIG_OPTION_OLD_CALCS */



  FT_EXPORT( FT_Int32 )  FT_SqrtFixed( FT_Int32  x );


#ifndef FT_CONFIG_OPTION_OLD_CALCS

#define SQRT_32( x )  FT_Sqrt32( x )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Sqrt32                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the square root of an Int32 integer (which will be        */
  /*    handled as an unsigned long value).                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x :: The value to compute the root for.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `sqrt(x)'.                                           */
  /*                                                                       */
  FT_EXPORT( FT_Int32 )
  FT_Sqrt32( FT_Int32  x );


#endif /* !FT_CONFIG_OPTION_OLD_CALCS */


  /*************************************************************************/
  /*                                                                       */
  /* FT_MulDiv() and FT_MulFix() are declared in freetype.h.               */
  /*                                                                       */
  /*************************************************************************/


#define INT_TO_F26DOT6( x )    ( (FT_Long)(x) << 6  )
#define INT_TO_F2DOT14( x )    ( (FT_Long)(x) << 14 )
#define INT_TO_FIXED( x )      ( (FT_Long)(x) << 16 )
#define F2DOT14_TO_FIXED( x )  ( (FT_Long)(x) << 2  )
#define FLOAT_TO_FIXED( x )    ( (FT_Long)( x * 65536.0 ) )

#define ROUND_F26DOT6( x )     ( x >= 0 ? (    ( (x) + 32 ) & -64 )     \
                                        : ( -( ( 32 - (x) ) & -64 ) ) )


FT_END_HEADER

#endif /* __FTCALC_H__ */


/* END */
