/***************************************************************************/
/*                                                                         */
/*  ttcmap.c                                                               */
/*                                                                         */
/*    TrueType character mapping table (cmap) support (body).              */
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
#include FT_INTERNAL_DEBUG_H
#include "ttload.h"
#include "ttcmap.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttcmap


  FT_CALLBACK_DEF( FT_UInt )
  code_to_index0( TT_CMapTable*  charmap,
                  FT_ULong       char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index2( TT_CMapTable*  charmap,
                  FT_ULong       char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index4( TT_CMapTable*  charmap,
                  FT_ULong       char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index6( TT_CMapTable*  charmap,
                  FT_ULong       char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index8_12( TT_CMapTable*  charmap,
                     FT_ULong       char_code );

  FT_CALLBACK_DEF( FT_UInt )
  code_to_index10( TT_CMapTable*  charmap,
                   FT_ULong       char_code );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_CharMap_Load                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given TrueType character map into memory.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the parent face object.                      */
  /*    stream :: A handle to the current stream object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    table  :: A pointer to a cmap object.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function assumes that the stream is already in use (i.e.,      */
  /*    opened).  In case of error, all partially allocated tables are     */
  /*    released.                                                          */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_CharMap_Load( TT_Face        face,
                   TT_CMapTable*  cmap,
                   FT_Stream      stream )
  {
    FT_Error      error;
    FT_Memory     memory;
    FT_UShort     num_SH, num_Seg, i;
    FT_ULong      j, n;

    FT_UShort     u, l;

    TT_CMap0*     cmap0;
    TT_CMap2*     cmap2;
    TT_CMap4*     cmap4;
    TT_CMap6*     cmap6;
    TT_CMap8_12*  cmap8_12;
    TT_CMap10*    cmap10;

    TT_CMap2SubHeader*  cmap2sub;
    TT_CMap4Segment*    segments;
    TT_CMapGroup*       groups;


    if ( cmap->loaded )
      return SFNT_Err_Ok;

    memory = stream->memory;

    if ( FILE_Seek( cmap->offset ) )
      return error;

    switch ( cmap->format )
    {
    case 0:
      cmap0 = &cmap->c.cmap0;

      if ( READ_UShort( cmap0->language )         ||
           ALLOC( cmap0->glyphIdArray, 256L )     ||
           FILE_Read( cmap0->glyphIdArray, 256L ) )
        goto Fail;

      cmap->get_index = code_to_index0;
      break;

    case 2:
      num_SH = 0;
      cmap2  = &cmap->c.cmap2;

      /* allocate subheader keys */

      if ( ALLOC_ARRAY( cmap2->subHeaderKeys, 256, FT_UShort ) ||
           ACCESS_Frame( 2L + 512L )                           )
        goto Fail;

      cmap2->language = GET_UShort();

      for ( i = 0; i < 256; i++ )
      {
        u = (FT_UShort)( GET_UShort() / 8 );
        cmap2->subHeaderKeys[i] = u;

        if ( num_SH < u )
          num_SH = u;
      }

      FORGET_Frame();

      /* load subheaders */

      cmap2->numGlyphId = l = (FT_UShort)(
        ( ( cmap->length - 2L * ( 256 + 3 ) - num_SH * 8L ) & 0xFFFF ) / 2 );

      if ( ALLOC_ARRAY( cmap2->subHeaders,
                        num_SH + 1,
                        TT_CMap2SubHeader )    ||
           ACCESS_Frame( ( num_SH + 1 ) * 8L ) )
      {
        FREE( cmap2->subHeaderKeys );
        goto Fail;
      }

      cmap2sub = cmap2->subHeaders;

      for ( i = 0; i <= num_SH; i++ )
      {
        cmap2sub->firstCode     = GET_UShort();
        cmap2sub->entryCount    = GET_UShort();
        cmap2sub->idDelta       = GET_Short();
        /* we apply the location offset immediately */
        cmap2sub->idRangeOffset = (FT_UShort)(
          GET_UShort() - ( num_SH - i ) * 8 - 2 );

        cmap2sub++;
      }

      FORGET_Frame();

      /* load glyph IDs */

      if ( ALLOC_ARRAY( cmap2->glyphIdArray, l, FT_UShort ) ||
           ACCESS_Frame( l * 2L )                           )
      {
        FREE( cmap2->subHeaders );
        FREE( cmap2->subHeaderKeys );
        goto Fail;
      }

      for ( i = 0; i < l; i++ )
        cmap2->glyphIdArray[i] = GET_UShort();

      FORGET_Frame();

      cmap->get_index = code_to_index2;
      break;

    case 4:
      cmap4 = &cmap->c.cmap4;

      /* load header */

      if ( ACCESS_Frame( 10L ) )
        goto Fail;

      cmap4->language      = GET_UShort();
      cmap4->segCountX2    = GET_UShort();
      cmap4->searchRange   = GET_UShort();
      cmap4->entrySelector = GET_UShort();
      cmap4->rangeShift    = GET_UShort();

      num_Seg = (FT_UShort)( cmap4->segCountX2 / 2 );

      FORGET_Frame();

      /* load segments */

      if ( ALLOC_ARRAY( cmap4->segments,
                        num_Seg,
                        TT_CMap4Segment )           ||
           ACCESS_Frame( ( num_Seg * 4 + 1 ) * 2L ) )
        goto Fail;

      segments = cmap4->segments;

      for ( i = 0; i < num_Seg; i++ )
        segments[i].endCount      = GET_UShort();

      (void)GET_UShort();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].startCount    = GET_UShort();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].idDelta       = GET_Short();

      for ( i = 0; i < num_Seg; i++ )
        segments[i].idRangeOffset = GET_UShort();

      FORGET_Frame();

      cmap4->numGlyphId = l = (FT_UShort)(
        ( ( cmap->length - ( 16L + 8L * num_Seg ) ) & 0xFFFF ) / 2 );

      /* load IDs */

      if ( ALLOC_ARRAY( cmap4->glyphIdArray, l, FT_UShort ) ||
           ACCESS_Frame( l * 2L )                           )
      {
        FREE( cmap4->segments );
        goto Fail;
      }

      for ( i = 0; i < l; i++ )
        cmap4->glyphIdArray[i] = GET_UShort();

      FORGET_Frame();

      cmap4->last_segment = cmap4->segments;

      cmap->get_index = code_to_index4;
      break;

    case 6:
      cmap6 = &cmap->c.cmap6;

      if ( ACCESS_Frame( 6L ) )
        goto Fail;

      cmap6->language   = GET_UShort();
      cmap6->firstCode  = GET_UShort();
      cmap6->entryCount = GET_UShort();

      FORGET_Frame();

      l = cmap6->entryCount;

      if ( ALLOC_ARRAY( cmap6->glyphIdArray, l, FT_Short ) ||
           ACCESS_Frame( l * 2L )                          )
        goto Fail;

      for ( i = 0; i < l; i++ )
        cmap6->glyphIdArray[i] = GET_UShort();

      FORGET_Frame();
      cmap->get_index = code_to_index6;
      break;

    case 8:
    case 12:
      cmap8_12 = &cmap->c.cmap8_12;

      if ( ACCESS_Frame( 8L ) )
        goto Fail;

      cmap->length       = GET_ULong();
      cmap8_12->language = GET_ULong();

      FORGET_Frame();

      if ( cmap->format == 8 )
        if ( FILE_Skip( 8192L ) )
          goto Fail;

      if ( READ_ULong( cmap8_12->nGroups ) )
        goto Fail;

      n = cmap8_12->nGroups;

      if ( ALLOC_ARRAY( cmap8_12->groups, n, TT_CMapGroup ) ||
           ACCESS_Frame( n * 3 * 4L )                       )
        goto Fail;

      groups = cmap8_12->groups;

      for ( j = 0; j < n; j++ )
      {
        groups[j].startCharCode = GET_ULong();
        groups[j].endCharCode   = GET_ULong();
        groups[j].startGlyphID  = GET_ULong();
      }

      FORGET_Frame();

      cmap8_12->last_group = cmap8_12->groups;

      cmap->get_index = code_to_index8_12;
      break;

    case 10:
      cmap10 = &cmap->c.cmap10;

      if ( ACCESS_Frame( 16L ) )
        goto Fail;

      cmap->length          = GET_ULong();
      cmap10->language      = GET_ULong();
      cmap10->startCharCode = GET_ULong();
      cmap10->numChars      = GET_ULong();

      FORGET_Frame();

      n = cmap10->numChars;

      if ( ALLOC_ARRAY( cmap10->glyphs, n, FT_Short ) ||
           ACCESS_Frame( n * 2L )                     )
        goto Fail;

      for ( j = 0; j < n; j++ )
        cmap10->glyphs[j] = GET_UShort();

      FORGET_Frame();
      cmap->get_index = code_to_index10;
      break;

    default:   /* corrupt character mapping table */
      return SFNT_Err_Invalid_CharMap_Format;

    }

    return SFNT_Err_Ok;

  Fail:
    TT_CharMap_Free( face, cmap );
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_CharMap_Free                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a character mapping table.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the parent face object.                        */
  /*    cmap :: A handle to a cmap object.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_CharMap_Free( TT_Face        face,
                   TT_CMapTable*  cmap )
  {
    FT_Memory  memory;


    if ( !cmap )
      return SFNT_Err_Ok;

    memory = face->root.driver->root.memory;

    switch ( cmap->format )
    {
    case 0:
      FREE( cmap->c.cmap0.glyphIdArray );
      break;

    case 2:
      FREE( cmap->c.cmap2.subHeaderKeys );
      FREE( cmap->c.cmap2.subHeaders );
      FREE( cmap->c.cmap2.glyphIdArray );
      break;

    case 4:
      FREE( cmap->c.cmap4.segments );
      FREE( cmap->c.cmap4.glyphIdArray );
      cmap->c.cmap4.segCountX2 = 0;
      break;

    case 6:
      FREE( cmap->c.cmap6.glyphIdArray );
      cmap->c.cmap6.entryCount = 0;
      break;

    case 8:
    case 12:
      FREE( cmap->c.cmap8_12.groups );
      cmap->c.cmap8_12.nGroups = 0;
      break;

    case 10:
      FREE( cmap->c.cmap10.glyphs );
      cmap->c.cmap10.numChars = 0;
      break;

    default:
      /* invalid table format, do nothing */
      ;
    }

    cmap->loaded = FALSE;
    return SFNT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index0                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 0.    */
  /*    `charCode' must be in the range 0x00-0xFF (otherwise 0 is          */
  /*    returned).                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap0    :: A pointer to a cmap table in format 0.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index0( TT_CMapTable*  cmap,
                  FT_ULong       charCode )
  {
    TT_CMap0*  cmap0 = &cmap->c.cmap0;


    return ( charCode <= 0xFF ? cmap0->glyphIdArray[charCode] : 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index2                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 2.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap2    :: A pointer to a cmap table in format 2.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index2( TT_CMapTable*  cmap,
                  FT_ULong       charCode )
  {
    FT_UInt             result, index1, offset;
    FT_UInt             char_lo;
    FT_ULong            char_hi;
    TT_CMap2SubHeader*  sh2;
    TT_CMap2*           cmap2;


    cmap2   = &cmap->c.cmap2;
    result  = 0;
    char_lo = (FT_UInt)( charCode & 0xFF );
    char_hi = charCode >> 8;

    if ( char_hi == 0 )
    {
      /* an 8-bit character code -- we use the subHeader 0 in this case */
      /* to test whether the character code is in the charmap           */
      index1 = cmap2->subHeaderKeys[char_lo];
      if ( index1 != 0 )
        return 0;
    }
    else
    {
      /* a 16-bit character code */
      index1 = cmap2->subHeaderKeys[char_hi & 0xFF];
      if ( index1 == 0 )
        return 0;
    }

    sh2      = cmap2->subHeaders + index1;
    char_lo -= sh2->firstCode;

    if ( char_lo < (FT_UInt)sh2->entryCount )
    {
      offset = sh2->idRangeOffset / 2 + char_lo;
      if ( offset < (FT_UInt)cmap2->numGlyphId )
      {
        result = cmap2->glyphIdArray[offset];
        if ( result )
          result = ( result + sh2->idDelta ) & 0xFFFF;
      }
    }

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index4                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 4.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap4    :: A pointer to a cmap table in format 4.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index4( TT_CMapTable*  cmap,
                  FT_ULong       charCode )
  {
    FT_UInt          result, index1, segCount;
    TT_CMap4*        cmap4;
    TT_CMap4Segment  *seg4, *limit;


    cmap4    = &cmap->c.cmap4;
    result   = 0;
    segCount = cmap4->segCountX2 / 2;
    limit    = cmap4->segments + segCount;

    /* first, check against the last used segment */

    seg4 = cmap4->last_segment;

    /* the following is equivalent to performing two tests, as in         */
    /*                                                                    */
    /*  if ( charCode >= seg4->startCount && charCode <= seg4->endCount ) */
    /*                                                                    */
    /* This is a bit strange, but it is faster, and the idea behind the   */
    /* cache is to significantly speed up charcode to glyph index         */
    /* conversion.                                                        */

    if ( (FT_ULong)( charCode       - seg4->startCount ) <
         (FT_ULong)( seg4->endCount - seg4->startCount ) )
      goto Found1;

    for ( seg4 = cmap4->segments; seg4 < limit; seg4++ )
    {
      /* the ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode > (FT_UInt)seg4->endCount )
        continue;

      if ( charCode >= (FT_UInt)seg4->startCount )
        goto Found;
    }
    return 0;

  Found:
    cmap4->last_segment = seg4;

  Found1:
    /* if the idRangeOffset is 0, we can compute the glyph index */
    /* directly                                                  */

    if ( seg4->idRangeOffset == 0 )
      result = ( charCode + seg4->idDelta ) & 0xFFFF;
    else
    {
      /* otherwise, we must use the glyphIdArray to do it */
      index1 = (FT_UInt)( seg4->idRangeOffset / 2
                          + ( charCode - seg4->startCount )
                          + ( seg4 - cmap4->segments )
                          - segCount );

      if ( index1 < (FT_UInt)cmap4->numGlyphId &&
           cmap4->glyphIdArray[index1] != 0    )
        result = ( cmap4->glyphIdArray[index1] + seg4->idDelta ) & 0xFFFF;
    }

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index6                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the character code into a glyph index.  Uses format 6.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap6    :: A pointer to a cmap table in format 6.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index6( TT_CMapTable*  cmap,
                  FT_ULong       charCode )
  {
    TT_CMap6*  cmap6;
    FT_UInt    result = 0;


    cmap6     = &cmap->c.cmap6;
    charCode -= cmap6->firstCode;

    if ( charCode < (FT_UInt)cmap6->entryCount )
      result = cmap6->glyphIdArray[charCode];

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index8_12                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the (possibly 32bit) character code into a glyph index.   */
  /*    Uses format 8 or 12.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap8_12 :: A pointer to a cmap table in format 8 or 12.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index8_12( TT_CMapTable*  cmap,
                     FT_ULong       charCode )
  {
    TT_CMap8_12*  cmap8_12;
    TT_CMapGroup  *group, *limit;


    cmap8_12 = &cmap->c.cmap8_12;
    limit    = cmap8_12->groups + cmap8_12->nGroups;

    /* first, check against the last used group */

    group = cmap8_12->last_group;

    /* the following is equivalent to performing two tests, as in       */
    /*                                                                  */
    /*  if ( charCode >= group->startCharCode &&                        */
    /*       charCode <= group->endCharCode   )                         */
    /*                                                                  */
    /* This is a bit strange, but it is faster, and the idea behind the */
    /* cache is to significantly speed up charcode to glyph index       */
    /* conversion.                                                      */

    if ( (FT_ULong)( charCode           - group->startCharCode ) <
         (FT_ULong)( group->endCharCode - group->startCharCode ) )
      goto Found1;

    for ( group = cmap8_12->groups; group < limit; group++ )
    {
      /* the ranges are sorted in increasing order.  If we are out of */
      /* the range here, the char code isn't in the charmap, so exit. */

      if ( charCode > group->endCharCode )
        continue;

      if ( charCode >= group->startCharCode )
        goto Found;
    }
    return 0;

  Found:
    cmap8_12->last_group = group;

  Found1:
    return group->startGlyphID + (FT_UInt)( charCode - group->startCharCode );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    code_to_index10                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts the (possibly 32bit) character code into a glyph index.   */
  /*    Uses format 10.                                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charCode :: The wanted character code.                             */
  /*    cmap10   :: A pointer to a cmap table in format 10.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index into the glyphs array.  0 if the glyph does not exist. */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_UInt )
  code_to_index10( TT_CMapTable*  cmap,
                   FT_ULong       charCode )
  {
    TT_CMap10*  cmap10;
    FT_UInt     result = 0;


    cmap10    = &cmap->c.cmap10;
    charCode -= cmap10->startCharCode;

    /* the overflow trick for comparison works here also since the number */
    /* of glyphs (even if numChars is specified as ULong in the specs) in */
    /* an OpenType font is limited to 64k                                 */

    if ( charCode < cmap10->numChars )
      result = cmap10->glyphs[charCode];

    return result;
  }


/* END */
