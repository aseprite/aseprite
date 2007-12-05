/***************************************************************************/
/*                                                                         */
/*  t1afm.c                                                                */
/*                                                                         */
/*    AFM support for Type 1 fonts (body).                                 */
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


#include <ft2build.h>
#include "t1afm.h"
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_TYPE1_TYPES_H
#include <stdlib.h>  /* for qsort()   */
#include <string.h>  /* for strcmp()  */
#include <ctype.h>   /* for isalnum() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1afm


  FT_LOCAL_DEF void
  T1_Done_AFM( FT_Memory  memory,
               T1_AFM*    afm )
  {
    FREE( afm->kern_pairs );
    afm->num_pairs = 0;
    FREE( afm );
  }


#undef  IS_KERN_PAIR
#define IS_KERN_PAIR( p )  ( p[0] == 'K' && p[1] == 'P' )

#define IS_ALPHANUM( c )  ( isalnum( c ) || \
                            c == '_'     || \
                            c == '.'     )


  /* read a glyph name and return the equivalent glyph index */
  static FT_UInt
  afm_atoindex( FT_Byte**  start,
                FT_Byte*   limit,
                T1_Font*   type1 )
  {
    FT_Byte*  p = *start;
    FT_Int    len;
    FT_UInt   result = 0;
    char      temp[64];


    /* skip whitespace */
    while ( ( *p == ' ' || *p == '\t' || *p == ':' || *p == ';' ) &&
            p < limit                                             )
      p++;
    *start = p;

    /* now, read glyph name */
    while ( IS_ALPHANUM( *p ) && p < limit )
      p++;

    len = (FT_Int)( p - *start );

    if ( len > 0 && len < 64 )
    {
      FT_Int  n;


      /* copy glyph name to intermediate array */
      MEM_Copy( temp, *start, len );
      temp[len] = 0;

      /* lookup glyph name in face array */
      for ( n = 0; n < type1->num_glyphs; n++ )
      {
        char*  gname = (char*)type1->glyph_names[n];


        if ( gname && gname[0] == temp[0] && strcmp( gname, temp ) == 0 )
        {
          result = n;
          break;
        }
      }
    }
    *start = p;
    return result;
  }


  /* read an integer */
  static int
  afm_atoi( FT_Byte**  start,
            FT_Byte*   limit )
  {
    FT_Byte*  p    = *start;
    int       sum  = 0;
    int       sign = 1;


    /* skip everything that is not a number */
    while ( p < limit && !isdigit( *p ) )
    {
      sign = 1;
      if ( *p == '-' )
        sign = -1;

      p++;
    }

    while ( p < limit && isdigit( *p ) )
    {
      sum = sum * 10 + ( *p - '0' );
      p++;
    }
    *start = p;

    return sum * sign;
  }


#undef  KERN_INDEX
#define KERN_INDEX( g1, g2 )  ( ( (FT_ULong)g1 << 16 ) | g2 )


  /* compare two kerning pairs */
  FT_CALLBACK_DEF( int )
  compare_kern_pairs( const void*  a,
                      const void*  b )
  {
    T1_Kern_Pair*  pair1 = (T1_Kern_Pair*)a;
    T1_Kern_Pair*  pair2 = (T1_Kern_Pair*)b;

    FT_ULong  index1 = KERN_INDEX( pair1->glyph1, pair1->glyph2 );
    FT_ULong  index2 = KERN_INDEX( pair2->glyph1, pair2->glyph2 );


    return ( index1 - index2 );
  }


  /* parse an AFM file -- for now, only read the kerning pairs */
  FT_LOCAL_DEF FT_Error
  T1_Read_AFM( FT_Face    t1_face,
               FT_Stream  stream )
  {
    FT_Error       error;
    FT_Memory      memory = stream->memory;
    FT_Byte*       start;
    FT_Byte*       limit;
    FT_Byte*       p;
    FT_Int         count = 0;
    T1_Kern_Pair*  pair;
    T1_Font*       type1 = &((T1_Face)t1_face)->type1;
    T1_AFM*        afm   = 0;


    if ( ACCESS_Frame( stream->size ) )
      return error;

    start = (FT_Byte*)stream->cursor;
    limit = (FT_Byte*)stream->limit;
    p     = start;

    /* we are now going to count the occurences of `KP' or `KPX' in */
    /* the AFM file                                                 */
    count = 0;
    for ( p = start; p < limit - 3; p++ )
    {
      if ( IS_KERN_PAIR( p ) )
        count++;
    }

    /* Actually, kerning pairs are simply optional! */
    if ( count == 0 )
      goto Exit;

    /* allocate the pairs */
    if ( ALLOC( afm, sizeof ( *afm ) )                       ||
         ALLOC_ARRAY( afm->kern_pairs, count, T1_Kern_Pair ) )
      goto Exit;

    /* now, read each kern pair */
    pair           = afm->kern_pairs;
    afm->num_pairs = count;

    /* save in face object */
    ((T1_Face)t1_face)->afm_data = afm;

    t1_face->face_flags |= FT_FACE_FLAG_KERNING;

    for ( p = start; p < limit - 3; p++ )
    {
      if ( IS_KERN_PAIR( p ) )
      {
        FT_Byte*  q;


        /* skip keyword (KP or KPX) */
        q = p + 2;
        if ( *q == 'X' )
          q++;

        pair->glyph1    = afm_atoindex( &q, limit, type1 );
        pair->glyph2    = afm_atoindex( &q, limit, type1 );
        pair->kerning.x = afm_atoi( &q, limit );

        pair->kerning.y = 0;
        if ( p[2] != 'X' )
          pair->kerning.y = afm_atoi( &q, limit );

        pair++;
      }
    }

    /* now, sort the kern pairs according to their glyph indices */
    qsort( afm->kern_pairs, count, sizeof ( T1_Kern_Pair ),
           compare_kern_pairs );

  Exit:
    if ( error )
      FREE( afm );

    FORGET_Frame();

    return error;
  }


  /* find the kerning for a given glyph pair */
  FT_LOCAL_DEF void
  T1_Get_Kerning( T1_AFM*     afm,
                  FT_UInt     glyph1,
                  FT_UInt     glyph2,
                  FT_Vector*  kerning )
  {
    T1_Kern_Pair  *min, *mid, *max;
    FT_ULong      index = KERN_INDEX( glyph1, glyph2 );


    /* simple binary search */
    min = afm->kern_pairs;
    max = min + afm->num_pairs - 1;

    while ( min <= max )
    {
      FT_ULong  midi;


      mid  = min + ( max - min ) / 2;
      midi = KERN_INDEX( mid->glyph1, mid->glyph2 );

      if ( midi == index )
      {
        *kerning = mid->kerning;
        return;
      }

      if ( midi < index )
        min = mid + 1;
      else
        max = mid - 1;
    }

    kerning->x = 0;
    kerning->y = 0;
  }


/* END */
