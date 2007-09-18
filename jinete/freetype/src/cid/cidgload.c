/***************************************************************************/
/*                                                                         */
/*  cidgload.c                                                             */
/*                                                                         */
/*    CID-keyed Type1 Glyph Loader (body).                                 */
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
#include "cidload.h"
#include "cidgload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_OUTLINE_H

#include "ciderrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cidgload


  FT_CALLBACK_DEF( FT_Error )
  cid_load_glyph( T1_Decoder*  decoder,
                  FT_UInt      glyph_index )
  {
    CID_Face   face = (CID_Face)decoder->builder.face;
    CID_Info*  cid  = &face->cid;
    FT_Byte*   p;
    FT_UInt    entry_len = cid->fd_bytes + cid->gd_bytes;
    FT_UInt    fd_select;
    FT_ULong   off1, glyph_len;
    FT_Stream  stream = face->root.stream;
    FT_Error   error  = 0;


    /* read the CID font dict index and charstring offset from the CIDMap */
    if ( FILE_Seek( cid->data_offset + cid->cidmap_offset +
                    glyph_index * entry_len )               ||
         ACCESS_Frame( 2 * entry_len )                      )
      goto Exit;

    p = (FT_Byte*)stream->cursor;
    fd_select = (FT_UInt) cid_get_offset( &p, (FT_Byte)cid->fd_bytes );
    off1      = (FT_ULong)cid_get_offset( &p, (FT_Byte)cid->gd_bytes );
    p        += cid->fd_bytes;
    glyph_len = cid_get_offset( &p, (FT_Byte)cid->gd_bytes ) - off1;

    FORGET_Frame();

    /* now, if the glyph is not empty, set up the subrs array, and parse */
    /* the charstrings                                                   */
    if ( glyph_len > 0 )
    {
      CID_FontDict*  dict;
      CID_Subrs*     cid_subrs = face->subrs + fd_select;
      FT_Byte*       charstring;
      FT_Memory      memory = face->root.memory;


      /* setup subrs */
      decoder->num_subrs = cid_subrs->num_subrs;
      decoder->subrs     = cid_subrs->code;
      decoder->subrs_len = 0;

      /* setup font matrix */
      dict                 = cid->font_dicts + fd_select;

      decoder->font_matrix = dict->font_matrix;
      decoder->font_offset = dict->font_offset;
      decoder->lenIV       = dict->private_dict.lenIV;

      /* the charstrings are encoded (stupid!)  */
      /* load the charstrings, then execute it  */

      if ( ALLOC( charstring, glyph_len ) )
        goto Exit;

      if ( !FILE_Read_At( cid->data_offset + off1, charstring, glyph_len ) )
      {
        FT_Int cs_offset;


        /* Adjustment for seed bytes. */
        cs_offset = ( decoder->lenIV >= 0 ? decoder->lenIV : 0 );

        /* Decrypt only if lenIV >= 0. */
        if ( decoder->lenIV >= 0 )
          cid_decrypt( charstring, glyph_len, 4330 );

        error = decoder->funcs.parse_charstrings( decoder,
                                                  charstring + cs_offset,
                                                  glyph_len  - cs_offset  );
      }

      FREE( charstring );
    }

  Exit:
    return error;
  }


#if 0


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
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


  FT_LOCAL_DEF FT_Error
  CID_Compute_Max_Advance( CID_Face  face,
                           FT_Int*   max_advance )
  {
    FT_Error    error;
    T1_Decoder  decoder;
    FT_Int      glyph_index;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;


    *max_advance = 0;

    /* Initialize load decoder */
    error = psaux->t1_decoder_funcs->init( &decoder,
                                           (FT_Face)face,
                                           0, /* size       */
                                           0, /* glyph slot */
                                           0, /* glyph names! XXX */
                                           0, /* blend == 0 */
                                           0, /* hinting == 0 */
                                           cid_load_glyph );
    if ( error )
      return error;

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* for each glyph, parse the glyph charstring and extract */
    /* the advance width                                      */
    for ( glyph_index = 0; glyph_index < face->root.num_glyphs;
          glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = cid_load_glyph( &decoder, glyph_index );
      /* ignore the error if one occurred - skip to next glyph */
    }

    *max_advance = decoder.builder.advance.x;

    return CID_Err_Ok;
  }


#endif /* 0 */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********               UNHINTED GLYPH LOADER                  *********/
  /**********                                                      *********/
  /**********    The following code is in charge of loading a      *********/
  /**********    single outline.  It completely ignores hinting    *********/
  /**********    and is used when FT_LOAD_NO_HINTING is set.       *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF FT_Error
  CID_Load_Glyph( CID_GlyphSlot  glyph,
                  CID_Size       size,
                  FT_Int         glyph_index,
                  FT_Int         load_flags )
  {
    FT_Error    error;
    T1_Decoder  decoder;
    CID_Face    face = (CID_Face)glyph->root.face;
    FT_Bool     hinting;

    PSAux_Interface*  psaux = (PSAux_Interface*)face->psaux;
    FT_Matrix         font_matrix;
    FT_Vector         font_offset;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = size->root.metrics.x_scale;
    glyph->y_scale = size->root.metrics.y_scale;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    glyph->root.format = ft_glyph_format_outline;

    {
      error = psaux->t1_decoder_funcs->init( &decoder,
                                             (FT_Face)face,
                                             (FT_Size)size,
                                             (FT_GlyphSlot)glyph,
                                             0, /* glyph names -- XXX */
                                             0, /* blend == 0 */
                                             hinting,
                                             cid_load_glyph );

      /* set up the decoder */
      decoder.builder.no_recurse = FT_BOOL(
        ( ( load_flags & FT_LOAD_NO_RECURSE ) != 0 ) );

      error = cid_load_glyph( &decoder, glyph_index );

      font_matrix = decoder.font_matrix;
      font_offset = decoder.font_offset;

      /* save new glyph tables */
      psaux->t1_decoder_funcs->done( &decoder );
    }

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

        internal->glyph_matrix         = font_matrix;
        internal->glyph_delta          = font_offset;
        internal->glyph_transformed    = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance          = decoder.builder.advance.x;
        glyph->root.linearHoriAdvance = decoder.builder.advance.x;
        glyph->root.internal->glyph_transformed = 0;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.linearVertAdvance = 0;
        glyph->root.format = ft_glyph_format_outline;

        if ( size && size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline, &font_matrix );

        FT_Outline_Translate( &glyph->root.outline,
                              font_offset.x,
                              font_offset.y );

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur = decoder.builder.base;
          FT_Vector*   vec = cur->points;
          FT_Fixed     x_scale = glyph->x_scale;
          FT_Fixed     y_scale = glyph->y_scale;


          /* First of all, scale the points */
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
          cbox.xMax  = ( cbox.xMax + 63 ) & -64;
          cbox.yMax  = ( cbox.yMax + 63 ) & -64;
        }

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;
      }
    }
    return error;
  }


/* END */
