/***************************************************************************/
/*                                                                         */
/*  t1gload.c                                                              */
/*                                                                         */
/*    Type 1 Glyph Loader (body).                                          */
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
#include "t1gload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_OUTLINE_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1errors.h"

#include <string.h>     /* for strcmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1gload


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********            COMPUTE THE MAXIMUM ADVANCE WIDTH         *********/
  /**********                                                      *********/
  /**********    The following code is in charge of computing      *********/
  /**********    the maximum advance width of the font.  It        *********/
  /**********    quickly processes each glyph charstring to        *********/
  /**********    extract the value from either a `sbw' or `seac'   *********/
  /**********    operator.                                         *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_Error )
  T1_Parse_Glyph( T1_Decoder*  decoder,
                  FT_UInt      glyph_index )
  {
    T1_Face   face  = (T1_Face)decoder->builder.face;
    T1_Font*  type1 = &face->type1;


    decoder->font_matrix = type1->font_matrix;
    decoder->font_offset = type1->font_offset;

    return decoder->funcs.parse_charstrings(
                      decoder,
                      type1->charstrings    [glyph_index],
                      type1->charstrings_len[glyph_index] );
  }


  FT_LOCAL_DEF FT_Error
  T1_Compute_Max_Advance( T1_Face  face,
                          FT_Int*  max_advance )
  {
    FT_Error          error;
    T1_Decoder        decoder;
    FT_Int            glyph_index;
    T1_Font*          type1 = &face->type1;
    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;


    *max_advance = 0;

    /* initialize load decoder */
    error = psaux->t1_decoder_funcs->init( &decoder,
                                           (FT_Face)face,
                                           0, /* size       */
                                           0, /* glyph slot */
                                           (FT_Byte**)type1->glyph_names,
                                           face->blend,
                                           0,
                                           T1_Parse_Glyph );
    if ( error )
      return error;

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    decoder.num_subrs = type1->num_subrs;
    decoder.subrs     = type1->subrs;
    decoder.subrs_len = type1->subrs_len;

    /* for each glyph, parse the glyph charstring and extract */
    /* the advance width                                      */
    for ( glyph_index = 0; glyph_index < type1->num_glyphs; glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = T1_Parse_Glyph( &decoder, glyph_index );
      /* ignore the error if one occured - skip to next glyph */
    }

    *max_advance = decoder.builder.advance.x;
    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********               UNHINTED GLYPH LOADER                  *********/
  /**********                                                      *********/
  /**********    The following code is in charge of loading a      *********/
  /**********    single outline.  It completely ignores hinting    *********/
  /**********    and is used when FT_LOAD_NO_HINTING is set.       *********/
  /**********                                                      *********/
  /**********      The Type 1 hinter is located in `t1hint.c'      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF FT_Error
  T1_Load_Glyph( T1_GlyphSlot  glyph,
                 T1_Size       size,
                 FT_Int        glyph_index,
                 FT_Int        load_flags )
  {
    FT_Error                error;
    T1_Decoder              decoder;
    T1_Face                 face = (T1_Face)glyph->root.face;
    FT_Bool                 hinting;
    T1_Font*                type1         = &face->type1;
    PSAux_Interface*        psaux         = (PSAux_Interface*)face->psaux;
    const T1_Decoder_Funcs* decoder_funcs = psaux->t1_decoder_funcs;

    FT_Matrix               font_matrix;
    FT_Vector               font_offset;

    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = size->root.metrics.x_scale;
    glyph->y_scale = size->root.metrics.y_scale;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    glyph->root.format = ft_glyph_format_outline;

    error = decoder_funcs->init( &decoder,
                                 (FT_Face)face,
                                 (FT_Size)size,
                                 (FT_GlyphSlot)glyph,
                                 (FT_Byte**)type1->glyph_names,
                                 face->blend,
                                 FT_BOOL( hinting ),
                                 T1_Parse_Glyph );
    if ( error )
      goto Exit;

    decoder.builder.no_recurse = FT_BOOL(
                                   ( load_flags & FT_LOAD_NO_RECURSE ) != 0 );

    decoder.num_subrs = type1->num_subrs;
    decoder.subrs     = type1->subrs;
    decoder.subrs_len = type1->subrs_len;


    /* now load the unscaled outline */
    error = T1_Parse_Glyph( &decoder, glyph_index );
    if ( error )
      goto Exit;

    font_matrix = decoder.font_matrix;
    font_offset = decoder.font_offset;

    /* save new glyph tables */
    decoder_funcs->done( &decoder );

    /* now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax                                    */
    if ( !error )
    {
      glyph->root.outline.flags &= ft_outline_owner;
      glyph->root.outline.flags |= ft_outline_reverse_fill;

      /* for composite glyphs, return only left side bearing and */
      /* advance width                                           */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_Slot_Internal  internal = glyph->root.internal;


        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.builder.advance.x;
        internal->glyph_matrix           = font_matrix;
        internal->glyph_delta            = font_offset;
        internal->glyph_transformed      = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance                    = decoder.builder.advance.x;
        glyph->root.linearHoriAdvance           = decoder.builder.advance.x;
        glyph->root.internal->glyph_transformed = 0;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.linearVertAdvance = 0;

        glyph->root.format = ft_glyph_format_outline;

        if ( size && size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

#if 1
        /* apply the font matrix, if any */
        FT_Outline_Transform( &glyph->root.outline, &font_matrix );

        FT_Outline_Translate( &glyph->root.outline,
                              font_offset.x,
                              font_offset.y );
#endif

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur = decoder.builder.base;
          FT_Vector*   vec = cur->points;
          FT_Fixed     x_scale = glyph->x_scale;
          FT_Fixed     y_scale = glyph->y_scale;


          /* First of all, scale the points, if we are not hinting */
          if ( !hinting )
            for ( n = cur->n_points; n > 0; n--, vec++ )
            {
              vec->x = FT_MulFix( vec->x, x_scale );
              vec->y = FT_MulFix( vec->y, y_scale );
            }

          FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

          /* Then scale the metrics */
          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,  x_scale );
          metrics->vertAdvance  = FT_MulFix( metrics->vertAdvance,  y_scale );

          metrics->vertBearingX = FT_MulFix( metrics->vertBearingX, x_scale );
          metrics->vertBearingY = FT_MulFix( metrics->vertBearingY, y_scale );

          if ( hinting )
          {
            metrics->horiAdvance = ( metrics->horiAdvance + 32 ) & -64;
            metrics->vertAdvance = ( metrics->vertAdvance + 32 ) & -64;

            metrics->vertBearingX = ( metrics->vertBearingX + 32 ) & -64;
            metrics->vertBearingY = ( metrics->vertBearingY + 32 ) & -64;
          }
        }

        /* compute the other metrics */
        FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

        /* grid fit the bounding box if necessary */
        if ( hinting )
        {
          cbox.xMin &= -64;
          cbox.yMin &= -64;
          cbox.xMax  = ( cbox.xMax+63 ) & -64;
          cbox.yMax  = ( cbox.yMax+63 ) & -64;
        }

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;
      }

      /* Set control data to the glyph charstrings.  Note that this is */
      /* _not_ zero-terminated.                                        */
      glyph->root.control_data = type1->charstrings    [glyph_index];
      glyph->root.control_len  = type1->charstrings_len[glyph_index];
    }

  Exit:
    return error;
  }


/* END */
