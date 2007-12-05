/***************************************************************************/
/*                                                                         */
/*  ttgload.c                                                              */
/*                                                                         */
/*    TrueType Glyph Loader (body).                                        */
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
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_TAGS_H
#include FT_OUTLINE_H

#include "ttgload.h"

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttgload


  /*************************************************************************/
  /*                                                                       */
  /* Composite font flags.                                                 */
  /*                                                                       */
#define ARGS_ARE_WORDS       0x001
#define ARGS_ARE_XY_VALUES   0x002
#define ROUND_XY_TO_GRID     0x004
#define WE_HAVE_A_SCALE      0x008
/* reserved                  0x010 */
#define MORE_COMPONENTS      0x020
#define WE_HAVE_AN_XY_SCALE  0x040
#define WE_HAVE_A_2X2        0x080
#define WE_HAVE_INSTR        0x100
#define USE_MY_METRICS       0x200



  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Get_Metrics                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the horizontal or vertical metrics in font units for a     */
  /*    given glyph.  The metrics are the left side bearing (resp. top     */
  /*    side bearing) and advance width (resp. advance height).            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    header  :: A pointer to either the horizontal or vertical metrics  */
  /*               structure.                                              */
  /*                                                                       */
  /*    index   :: The glyph index.                                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    bearing :: The bearing, either left side or top side.              */
  /*                                                                       */
  /*    advance :: The advance width resp. advance height.                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will much probably move to another component in the  */
  /*    near future, but I haven't decided which yet.                      */
  /*                                                                       */
  FT_LOCAL_DEF void
  TT_Get_Metrics( TT_HoriHeader*  header,
                  FT_UInt         index,
                  FT_Short*       bearing,
                  FT_UShort*      advance )
  {
    TT_LongMetrics*  longs_m;
    FT_UShort        k = header->number_Of_HMetrics;


    if ( index < (FT_UInt)k )
    {
      longs_m  = (TT_LongMetrics*)header->long_metrics + index;
      *bearing = longs_m->bearing;
      *advance = longs_m->advance;
    }
    else
    {
      *bearing = ((TT_ShortMetrics*)header->short_metrics)[index - k];
      *advance = ((TT_LongMetrics*)header->long_metrics)[k - 1].advance;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* Returns the horizontal metrics in font units for a given glyph.  If   */
  /* `check' is true, take care of monospaced fonts by returning the       */
  /* advance width maximum.                                                */
  /*                                                                       */
  static void
  Get_HMetrics( TT_Face     face,
                FT_UInt     index,
                FT_Bool     check,
                FT_Short*   lsb,
                FT_UShort*  aw )
  {
    TT_Get_Metrics( &face->horizontal, index, lsb, aw );

    if ( check && face->postscript.isFixedPitch )
      *aw = face->horizontal.advance_Width_Max;
  }


  /*************************************************************************/
  /*                                                                       */
  /*    Returns the advance width table for a given pixel size if it is    */
  /*    found in the font's `hdmx' table (if any).                         */
  /*                                                                       */
  static FT_Byte*
  Get_Advance_Widths( TT_Face    face,
                      FT_UShort  ppem )
  {
    FT_UShort  n;

    for ( n = 0; n < face->hdmx.num_records; n++ )
      if ( face->hdmx.records[n].ppem == ppem )
        return face->hdmx.records[n].widths;

    return NULL;
  }


#define cur_to_org( n, zone ) \
          MEM_Copy( (zone)->org, (zone)->cur, (n) * sizeof ( FT_Vector ) )

#define org_to_cur( n, zone ) \
          MEM_Copy( (zone)->cur, (zone)->org, (n) * sizeof ( FT_Vector ) )


  /*************************************************************************/
  /*                                                                       */
  /*    Translates an array of coordinates.                                */
  /*                                                                       */
  static void
  translate_array( FT_UInt     n,
                   FT_Vector*  coords,
                   FT_Pos      delta_x,
                   FT_Pos      delta_y )
  {
    FT_UInt  k;


    if ( delta_x )
      for ( k = 0; k < n; k++ )
        coords[k].x += delta_x;

    if ( delta_y )
      for ( k = 0; k < n; k++ )
        coords[k].y += delta_y;
  }


  static void
  tt_prepare_zone( TT_GlyphZone*  zone,
                   FT_GlyphLoad*  load,
                   FT_UInt        start_point,
                   FT_UInt        start_contour )
  {
    zone->n_points   = (FT_UShort)( load->outline.n_points - start_point );
    zone->n_contours = (FT_Short) ( load->outline.n_contours - start_contour );
    zone->org        = load->extra_points + start_point;
    zone->cur        = load->outline.points + start_point;
    zone->tags       = (FT_Byte*)load->outline.tags + start_point;
    zone->contours   = (FT_UShort*)load->outline.contours + start_contour;
  }


#undef  IS_HINTED
#define IS_HINTED( flags )  ( ( flags & FT_LOAD_NO_HINTING ) == 0 )


  /*************************************************************************/
  /*                                                                       */
  /*  The following functions are used by default with TrueType fonts.     */
  /*  However, they can be replaced by alternatives if we need to support  */
  /*  TrueType-compressed formats (like MicroType) in the future.          */
  /*                                                                       */
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  TT_Access_Glyph_Frame( TT_Loader*  loader,
                         FT_UInt     glyph_index,
                         FT_ULong    offset,
                         FT_UInt     byte_count )
  {
    FT_Error   error;
    FT_Stream  stream = loader->stream;

    /* for non-debug mode */
    FT_UNUSED( glyph_index );


    FT_TRACE5(( "Glyph %ld\n", glyph_index ));

    /* the following line sets the `error' variable through macros! */
    if ( FILE_Seek( offset ) || ACCESS_Frame( byte_count ) )
      return error;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( void )
  TT_Forget_Glyph_Frame( TT_Loader*  loader )
  {
    FT_Stream  stream = loader->stream;


    FORGET_Frame();
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Glyph_Header( TT_Loader*  loader )
  {
    FT_Stream   stream   = loader->stream;
    FT_Int      byte_len = loader->byte_len - 10;


    if ( byte_len < 0 )
      return TT_Err_Invalid_Outline;

    loader->n_contours = GET_Short();

    loader->bbox.xMin = GET_Short();
    loader->bbox.yMin = GET_Short();
    loader->bbox.xMax = GET_Short();
    loader->bbox.yMax = GET_Short();

    FT_TRACE5(( "  # of contours: %d\n", loader->n_contours ));
    FT_TRACE5(( "  xMin: %4d  xMax: %4d\n", loader->bbox.xMin,
                                            loader->bbox.xMax ));
    FT_TRACE5(( "  yMin: %4d  yMax: %4d\n", loader->bbox.yMin,
                                            loader->bbox.yMax ));
    loader->byte_len = byte_len;

    return TT_Err_Ok;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Simple_Glyph( TT_Loader*  load )
  {
    FT_Error         error;
    FT_Stream        stream     = load->stream;
    FT_GlyphLoader*  gloader    = load->gloader;
    FT_Int           n_contours = load->n_contours;
    FT_Outline*      outline;
    TT_Face          face    = (TT_Face)load->face;
    TT_GlyphSlot     slot    = (TT_GlyphSlot)load->glyph;
    FT_UShort        n_ins;
    FT_Int           n, n_points;
    FT_Int           byte_len = load->byte_len;


    /* reading the contours endpoints & number of points */
    {
      short*  cur   = gloader->current.outline.contours;
      short*  limit = cur + n_contours;


      /* check space for contours array + instructions count */
      byte_len -= 2 * ( n_contours + 1 );
      if ( byte_len < 0 )
        goto Invalid_Outline;

      for ( ; cur < limit; cur++ )
        cur[0] = GET_UShort();

      n_points = 0;
      if ( n_contours > 0 )
        n_points = cur[-1] + 1;

      error = FT_GlyphLoader_Check_Points( gloader, n_points + 2, 0 );
      if ( error )
        goto Fail;

      /* we'd better check the contours table right now */
      outline = &gloader->current.outline;

      for ( cur = outline->contours + 1; cur < limit; cur++ )
        if ( cur[-1] >= cur[0] )
          goto Invalid_Outline;
    }

    /* reading the bytecode instructions */
    slot->control_len  = 0;
    slot->control_data = 0;

    n_ins = GET_UShort();

    FT_TRACE5(( "  Instructions size: %d\n", n_ins ));

    if ( n_ins > face->max_profile.maxSizeOfInstructions )
    {
      FT_TRACE0(( "ERROR: Too many instructions!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

    byte_len -= n_ins;
    if ( byte_len < 0 )
    {
      FT_TRACE0(( "ERROR: Instruction count mismatch!\n" ));
      error = TT_Err_Too_Many_Hints;
      goto Fail;
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( ( load->load_flags                        &
         ( FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) ) == 0 &&
           load->instructions )
    {
      slot->control_len  = n_ins;
      slot->control_data = load->instructions;

      MEM_Copy( load->instructions, stream->cursor, n_ins );
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    stream->cursor += n_ins;

    /* reading the point tags */
    {
      FT_Byte*  flag  = (FT_Byte*)outline->tags;
      FT_Byte*  limit = flag + n_points;
      FT_Byte   c, count;


      while ( flag < limit )
      {
        if ( --byte_len < 0 )
          goto Invalid_Outline;

        *flag++ = c = GET_Byte();
        if ( c & 8 )
        {
          if ( --byte_len < 0 )
            goto Invalid_Outline;

          count = GET_Byte();
          if ( flag + count > limit )
            goto Invalid_Outline;

          for ( ; count > 0; count-- )
            *flag++ = c;
        }
      }

      /* check that there is enough room to load the coordinates */
      for ( flag = (FT_Byte*)outline->tags; flag < limit; flag++ )
      {
        if ( *flag & 2 )
          byte_len -= 1;
        else if ( ( *flag & 16 ) == 0 )
          byte_len -= 2;

        if ( *flag & 4 )
          byte_len -= 1;
        else if ( ( *flag & 32 ) == 0 )
          byte_len -= 2;
      }

      if ( byte_len < 0 )
        goto Invalid_Outline;
    }

    /* reading the X coordinates */

    {
      FT_Vector*  vec   = outline->points;
      FT_Vector*  limit = vec + n_points;
      FT_Byte*    flag  = (FT_Byte*)outline->tags;
      FT_Pos      x     = 0;


      for ( ; vec < limit; vec++, flag++ )
      {
        FT_Pos  y = 0;


        if ( *flag & 2 )
        {
          y = GET_Byte();
          if ( ( *flag & 16 ) == 0 )
            y = -y;
        }
        else if ( ( *flag & 16 ) == 0 )
          y = GET_Short();

        x     += y;
        vec->x = x;
      }
    }

    /* reading the Y coordinates */

    {
      FT_Vector*  vec   = gloader->current.outline.points;
      FT_Vector*  limit = vec + n_points;
      FT_Byte*    flag  = (FT_Byte*)outline->tags;
      FT_Pos      x     = 0;


      for ( ; vec < limit; vec++, flag++ )
      {
        FT_Pos  y = 0;


        if ( *flag & 4 )
        {
          y = GET_Byte();
          if ( ( *flag & 32 ) == 0 )
            y = -y;
        }
        else if ( ( *flag & 32 ) == 0 )
          y = GET_Short();

        x     += y;
        vec->y = x;
      }
    }

    /* clear the touch tags */
    for ( n = 0; n < n_points; n++ )
      outline->tags[n] &= FT_Curve_Tag_On;

    outline->n_points   = (FT_UShort)n_points;
    outline->n_contours = (FT_Short) n_contours;

    load->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Outline:
    error = TT_Err_Invalid_Outline;
    goto Fail;
  }


  FT_CALLBACK_DEF( FT_Error )
  TT_Load_Composite_Glyph( TT_Loader*  loader )
  {
    FT_Error         error;
    FT_Stream        stream  = loader->stream;
    FT_GlyphLoader*  gloader = loader->gloader;
    FT_SubGlyph*     subglyph;
    FT_UInt          num_subglyphs;
    FT_Int           byte_len = loader->byte_len;


    num_subglyphs = 0;

    do
    {
      FT_Fixed  xx, xy, yy, yx;


      /* check that we can load a new subglyph */
      error = FT_GlyphLoader_Check_Subglyphs( gloader, num_subglyphs + 1 );
      if ( error )
        goto Fail;

      /* check space */
      byte_len -= 4;
      if ( byte_len < 0 )
        goto Invalid_Composite;

      subglyph = gloader->current.subglyphs + num_subglyphs;

      subglyph->arg1 = subglyph->arg2 = 0;

      subglyph->flags = GET_UShort();
      subglyph->index = GET_UShort();

      /* check space */
      byte_len -= 2;
      if ( subglyph->flags & ARGS_ARE_WORDS )
        byte_len -= 2;
      if ( subglyph->flags & WE_HAVE_A_SCALE )
        byte_len -= 2;
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
        byte_len -= 4;
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
        byte_len -= 8;

      if ( byte_len < 0 )
        goto Invalid_Composite;

      /* read arguments */
      if ( subglyph->flags & ARGS_ARE_WORDS )
      {
        subglyph->arg1 = GET_Short();
        subglyph->arg2 = GET_Short();
      }
      else
      {
        subglyph->arg1 = GET_Char();
        subglyph->arg2 = GET_Char();
      }

      /* read transform */
      xx = yy = 0x10000L;
      xy = yx = 0;

      if ( subglyph->flags & WE_HAVE_A_SCALE )
      {
        xx = (FT_Fixed)GET_Short() << 2;
        yy = xx;
      }
      else if ( subglyph->flags & WE_HAVE_AN_XY_SCALE )
      {
        xx = (FT_Fixed)GET_Short() << 2;
        yy = (FT_Fixed)GET_Short() << 2;
      }
      else if ( subglyph->flags & WE_HAVE_A_2X2 )
      {
        xx = (FT_Fixed)GET_Short() << 2;
        xy = (FT_Fixed)GET_Short() << 2;
        yx = (FT_Fixed)GET_Short() << 2;
        yy = (FT_Fixed)GET_Short() << 2;
      }

      subglyph->transform.xx = xx;
      subglyph->transform.xy = xy;
      subglyph->transform.yx = yx;
      subglyph->transform.yy = yy;

      num_subglyphs++;

    } while ( subglyph->flags & MORE_COMPONENTS );

    gloader->current.num_subglyphs = num_subglyphs;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    {
      /* we must undo the ACCESS_Frame in order to point to the */
      /* composite instructions, if we find some.               */
      /* we will process them later...                          */
      /*                                                        */
      loader->ins_pos = (FT_ULong)( FILE_Pos() +
                                    stream->cursor - stream->limit );
    }
#endif

    loader->byte_len = byte_len;

  Fail:
    return error;

  Invalid_Composite:
    error = TT_Err_Invalid_Composite;
    goto Fail;
  }


  FT_LOCAL_DEF void
  TT_Init_Glyph_Loading( TT_Face  face )
  {
    face->access_glyph_frame   = TT_Access_Glyph_Frame;
    face->read_glyph_header    = TT_Load_Glyph_Header;
    face->read_simple_glyph    = TT_Load_Simple_Glyph;
    face->read_composite_glyph = TT_Load_Composite_Glyph;
    face->forget_glyph_frame   = TT_Forget_Glyph_Frame;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Process_Simple_Glyph                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Once a simple glyph has been loaded, it needs to be processed.     */
  /*    Usually, this means scaling and hinting through bytecode           */
  /*    interpretation.                                                    */
  /*                                                                       */
  static FT_Error
  TT_Process_Simple_Glyph( TT_Loader*  load,
                           FT_Bool     debug )
  {
    FT_GlyphLoader*  gloader  = load->gloader;
    FT_Outline*      outline  = &gloader->current.outline;
    FT_UInt          n_points = outline->n_points;
    FT_UInt          n_ins;
    TT_GlyphZone*    zone     = &load->zone;
    FT_Error         error    = TT_Err_Ok;

    FT_UNUSED( debug );  /* used by truetype interpreter only */


    n_ins = load->glyph->control_len;

    /* add shadow points */

    /* Now add the two shadow points at n and n + 1.    */
    /* We need the left side bearing and advance width. */

    {
      FT_Vector*  pp1;
      FT_Vector*  pp2;


      /* pp1 = xMin - lsb */
      pp1    = outline->points + n_points;
      pp1->x = load->bbox.xMin - load->left_bearing;
      pp1->y = 0;

      /* pp2 = pp1 + aw */
      pp2    = pp1 + 1;
      pp2->x = pp1->x + load->advance;
      pp2->y = 0;

      outline->tags[n_points    ] = 0;
      outline->tags[n_points + 1] = 0;
    }

    /* Note that we return two more points that are not */
    /* part of the glyph outline.                       */

    n_points += 2;

    /* set up zone for hinting */
    tt_prepare_zone( zone, &gloader->current, 0, 0 );

    /* eventually scale the glyph */
    if ( !( load->load_flags & FT_LOAD_NO_SCALE ) )
    {
      FT_Vector*  vec     = zone->cur;
      FT_Vector*  limit   = vec + n_points;
      FT_Fixed    x_scale = load->size->metrics.x_scale;
      FT_Fixed    y_scale = load->size->metrics.y_scale;


      /* first scale the glyph points */
      for ( ; vec < limit; vec++ )
      {
        vec->x = FT_MulFix( vec->x, x_scale );
        vec->y = FT_MulFix( vec->y, y_scale );
      }
    }

    cur_to_org( n_points, zone );

    /* eventually hint the glyph */
    if ( IS_HINTED( load->load_flags ) )
    {
      FT_Pos  x = zone->org[n_points-2].x;


      x = ( ( x + 32 ) & -64 ) - x;
      translate_array( n_points, zone->org, x, 0 );

      org_to_cur( n_points, zone );

      zone->cur[n_points - 1].x = ( zone->cur[n_points - 1].x + 32 ) & -64;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      /* now consider hinting */
      if ( n_ins > 0 )
      {
        error = TT_Set_CodeRange( load->exec, tt_coderange_glyph,
                                  load->exec->glyphIns, n_ins );
        if ( error )
          goto Exit;

        load->exec->is_composite     = FALSE;
        load->exec->pedantic_hinting = (FT_Bool)( load->load_flags &
                                                  FT_LOAD_PEDANTIC );
        load->exec->pts              = *zone;
        load->exec->pts.n_points    += 2;

        error = TT_Run_Context( load->exec, debug );
        if ( error && load->exec->pedantic_hinting )
          goto Exit;

        error = TT_Err_Ok;  /* ignore bytecode errors in non-pedantic mode */
      }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    }

    /* save glyph phantom points */
    if ( !load->preserve_pps )
    {
      load->pp1 = zone->cur[n_points - 2];
      load->pp2 = zone->cur[n_points - 1];
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
  Exit:
#endif
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    load_truetype_glyph                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given truetype glyph.  Handles composites and uses a       */
  /*    TT_Loader object.                                                  */
  /*                                                                       */
  static FT_Error
  load_truetype_glyph( TT_Loader*  loader,
                       FT_UInt     glyph_index )
  {

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
    FT_Stream        stream = loader->stream;
#endif

    FT_Error         error;
    TT_Face          face   = (TT_Face)loader->face;
    FT_ULong         offset;
    FT_Int           contours_count;
    FT_UInt          index, num_points, count;
    FT_Fixed         x_scale, y_scale;
    FT_GlyphLoader*  gloader = loader->gloader;
    FT_Bool          opened_frame = 0;


    /* check glyph index */
    index = glyph_index;
    if ( index >= (FT_UInt)face->root.num_glyphs )
    {
      error = TT_Err_Invalid_Glyph_Index;
      goto Exit;
    }

    loader->glyph_index = glyph_index;
    num_points   = 0;

    x_scale = 0x10000L;
    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      x_scale = loader->size->metrics.x_scale;
      y_scale = loader->size->metrics.y_scale;
    }

    /* get horizontal metrics */
    {
      FT_Short   left_bearing;
      FT_UShort  advance_width;


      Get_HMetrics( face, index,
                    (FT_Bool)!(loader->load_flags &
                                FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH),
                    &left_bearing,
                    &advance_width );

      loader->left_bearing = left_bearing;
      loader->advance      = advance_width;

      if ( !loader->linear_def )
      {
        loader->linear_def = 1;
        loader->linear     = advance_width;
      }
    }

    offset = face->glyph_locations[index];
    count  = 0;

    if ( index < (FT_UInt)face->num_locations - 1 )
       count = face->glyph_locations[index + 1] - offset;

    if ( count == 0 )
    {
      /* as described by Frederic Loyer, these are spaces, and */
      /* not the unknown glyph.                                */
      loader->bbox.xMin = 0;
      loader->bbox.xMax = 0;
      loader->bbox.yMin = 0;
      loader->bbox.yMax = 0;

      loader->pp1.x = 0;
      loader->pp2.x = loader->advance;

      if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
        loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      if ( loader->exec )
        loader->exec->glyphSize = 0;

#endif

      error = TT_Err_Ok;
      goto Exit;
    }

    loader->byte_len = (FT_Int)count;

#if 0
    /* temporary hack */
    if ( count < 10 )
    {
      /* This glyph is corrupted -- it does not have a complete header */
      error = TT_Err_Invalid_Outline;
      goto Fail;
    }
#endif

    offset = loader->glyf_offset + offset;

    /* access glyph frame */
    error = face->access_glyph_frame( loader, glyph_index, offset, count );
    if ( error )
      goto Exit;

    opened_frame = 1;

    /* read first glyph header */
    error = face->read_glyph_header( loader );
    if ( error )
      goto Fail;

    contours_count = loader->n_contours;

    count -= 10;

    loader->pp1.x = loader->bbox.xMin - loader->left_bearing;
    loader->pp1.y = 0;
    loader->pp2.x = loader->pp1.x + loader->advance;
    loader->pp2.y = 0;

    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
    {
      loader->pp1.x = FT_MulFix( loader->pp1.x, x_scale );
      loader->pp2.x = FT_MulFix( loader->pp2.x, x_scale );
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* if it is a simple glyph, load it */

    if ( contours_count >= 0 )
    {
      /* check that we can add the contours to the glyph */
      error = FT_GlyphLoader_Check_Points( gloader, 0, contours_count );
      if ( error )
        goto Fail;

      error = face->read_simple_glyph( loader );
      if ( error )
        goto Fail;

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

      {
        TT_Size size = (TT_Size)loader->size;


        error = TT_Process_Simple_Glyph( loader,
                                         (FT_Bool)( size && size->debug ) );
      }

#else

      error = TT_Process_Simple_Glyph( loader, 0 );

#endif

      if ( error )
        goto Fail;

      FT_GlyphLoader_Add( gloader );

      /* Note: We could have put the simple loader source there */
      /*       but the code is fat enough already :-)           */
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

    /* otherwise, load a composite! */
    else
    {
      TT_GlyphSlot  glyph = (TT_GlyphSlot)loader->glyph;
      FT_UInt       start_point, start_contour;
      FT_ULong      ins_pos;  /* position of composite instructions, if any */


      /* for each subglyph, read composite header */
      start_point   = gloader->base.outline.n_points;
      start_contour = gloader->base.outline.n_contours;

      error = face->read_composite_glyph( loader );
      if ( error )
        goto Fail;

      ins_pos = loader->ins_pos;
      face->forget_glyph_frame( loader );
      opened_frame = 0;

      /* if the flag FT_LOAD_NO_RECURSE is set, we return the subglyph */
      /* `as is' in the glyph slot (the client application will be     */
      /* responsible for interpreting this data)...                    */
      /*                                                               */
      if ( loader->load_flags & FT_LOAD_NO_RECURSE )
      {
        /* set up remaining glyph fields */
        FT_GlyphLoader_Add( gloader );

        glyph->num_subglyphs  = gloader->base.num_subglyphs;
        glyph->format         = ft_glyph_format_composite;
        glyph->subglyphs      = gloader->base.subglyphs;

        goto Exit;
      }

      /*********************************************************************/
      /*********************************************************************/
      /*********************************************************************/

      /* Now, read each subglyph independently. */
      {
        FT_Int        n, num_base_points, num_new_points;
        FT_SubGlyph*  subglyph = 0;

        FT_UInt num_subglyphs  = gloader->current.num_subglyphs;
        FT_UInt num_base_subgs = gloader->base.num_subglyphs;


        FT_GlyphLoader_Add( gloader );

        for ( n = 0; n < (FT_Int)num_subglyphs; n++ )
        {
          FT_Vector  pp1, pp2;
          FT_Pos     x, y;


          /* Each time we call load_truetype_glyph in this loop, the   */
          /* value of `gloader.base.subglyphs' can change due to table */
          /* reallocations.  We thus need to recompute the subglyph    */
          /* pointer on each iteration.                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          pp1 = loader->pp1;
          pp2 = loader->pp2;

          num_base_points = gloader->base.outline.n_points;

          error = load_truetype_glyph( loader, subglyph->index );
          if ( error )
            goto Fail;

          /* restore subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + n;

          if ( subglyph->flags & USE_MY_METRICS )
          {
            pp1 = loader->pp1;
            pp2 = loader->pp2;
          }
          else
          {
            loader->pp1 = pp1;
            loader->pp2 = pp2;
          }

          num_points = gloader->base.outline.n_points;

          num_new_points = num_points - num_base_points;

          /* now perform the transform required for this subglyph */

          if ( subglyph->flags & ( WE_HAVE_A_SCALE     |
                                   WE_HAVE_AN_XY_SCALE |
                                   WE_HAVE_A_2X2       ) )
          {
            FT_Vector*  cur   = gloader->base.outline.points +
                                  num_base_points;
            FT_Vector*  org   = gloader->base.extra_points +
                                  num_base_points;
            FT_Vector*  limit = cur + num_new_points;


            for ( ; cur < limit; cur++, org++ )
            {
              FT_Vector_Transform( cur, &subglyph->transform );
              FT_Vector_Transform( org, &subglyph->transform );
            }
          }

          /* apply offset */

          if ( !( subglyph->flags & ARGS_ARE_XY_VALUES ) )
          {
            FT_UInt     k = subglyph->arg1;
            FT_UInt     l = subglyph->arg2;
            FT_Vector*  p1;
            FT_Vector*  p2;


            if ( start_point + k >= (FT_UInt)num_base_points ||
                               l >= (FT_UInt)num_new_points  )
            {
              error = TT_Err_Invalid_Composite;
              goto Fail;
            }

            l += num_base_points;

            p1 = gloader->base.outline.points + start_point + k;
            p2 = gloader->base.outline.points + start_point + l;

            x = p1->x - p2->x;
            y = p1->y - p2->y;
          }
          else
          {
            x = subglyph->arg1;
            y = subglyph->arg2;

            if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
            {
              x = FT_MulFix( x, x_scale );
              y = FT_MulFix( y, y_scale );

              if ( subglyph->flags & ROUND_XY_TO_GRID )
              {
                x = ( x + 32 ) & -64;
                y = ( y + 32 ) & -64;
              }
            }
          }

          if ( x | y )
          {
            translate_array( num_new_points,
                             gloader->base.outline.points + num_base_points,
                             x, y );

            translate_array( num_new_points,
                             gloader->base.extra_points + num_base_points,
                             x, y );
          }
        }

        /*******************************************************************/
        /*******************************************************************/
        /*******************************************************************/

        /* we have finished loading all sub-glyphs; now, look for */
        /* instructions for this composite!                       */

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

        if ( num_subglyphs > 0               &&
             loader->exec                    &&
             ins_pos         > 0             &&
             subglyph->flags & WE_HAVE_INSTR )
        {
          FT_UShort       n_ins;
          TT_ExecContext  exec = loader->exec;
          TT_GlyphZone*   pts;
          FT_Vector*      pp1;


          /* read size of instructions */
          if ( FILE_Seek( ins_pos ) ||
               READ_UShort( n_ins ) )
            goto Fail;
          FT_TRACE5(( "  Instructions size = %d\n", n_ins ));

          /* in some fonts? */
          if ( n_ins == 0xFFFF )
            n_ins = 0;

          /* check it */
          if ( n_ins > face->max_profile.maxSizeOfInstructions )
          {
            FT_TRACE0(( "Too many instructions (%d) in composite glyph %ld\n",
                        n_ins, subglyph->index ));
            error = TT_Err_Too_Many_Hints;
            goto Fail;
          }

          /* read the instructions */
          if ( FILE_Read( exec->glyphIns, n_ins ) )
            goto Fail;

          glyph->control_data = exec->glyphIns;
          glyph->control_len  = n_ins;

          error = TT_Set_CodeRange( exec,
                                    tt_coderange_glyph,
                                    exec->glyphIns,
                                    n_ins );
          if ( error )
            goto Fail;

          /* prepare the execution context */
          tt_prepare_zone( &exec->pts, &gloader->base,
                           start_point, start_contour );
          pts = &exec->pts;

          pts->n_points   = (short)(num_points + 2);
          pts->n_contours = gloader->base.outline.n_contours;

          /* add phantom points */
          pp1    = pts->cur + num_points;
          pp1[0] = loader->pp1;
          pp1[1] = loader->pp2;

          pts->tags[num_points    ] = 0;
          pts->tags[num_points + 1] = 0;

          /* if hinting, round the phantom points */
          if ( IS_HINTED( loader->load_flags ) )
          {
            pp1[0].x = ( ( loader->pp1.x + 32 ) & -64 );
            pp1[1].x = ( ( loader->pp2.x + 32 ) & -64 );
          }

          {
            FT_UInt  k;


            for ( k = 0; k < num_points; k++ )
              pts->tags[k] &= FT_Curve_Tag_On;
          }

          cur_to_org( num_points + 2, pts );

          /* now consider hinting */
          if ( IS_HINTED( loader->load_flags ) && n_ins > 0 )
          {
            exec->is_composite     = TRUE;
            exec->pedantic_hinting =
              (FT_Bool)( loader->load_flags & FT_LOAD_PEDANTIC );

            error = TT_Run_Context( exec, ((TT_Size)loader->size)->debug );
            if ( error && exec->pedantic_hinting )
              goto Fail;
          }

          /* save glyph origin and advance points */
          loader->pp1 = pp1[0];
          loader->pp2 = pp1[1];
        }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

      }
      /* end of composite loading */
    }

    /***********************************************************************/
    /***********************************************************************/
    /***********************************************************************/

  Fail:
    if ( opened_frame )
      face->forget_glyph_frame( loader );

  Exit:
    return error;
  }


  static void
  compute_glyph_metrics( TT_Loader*  loader,
                         FT_UInt     glyph_index )
  {
    FT_BBox       bbox;
    TT_Face       face = (TT_Face)loader->face;
    FT_Fixed      y_scale;
    TT_GlyphSlot  glyph = loader->glyph;
    TT_Size       size = (TT_Size)loader->size;


    y_scale = 0x10000L;
    if ( ( loader->load_flags & FT_LOAD_NO_SCALE ) == 0 )
      y_scale = size->root.metrics.y_scale;

    if ( glyph->format != ft_glyph_format_composite )
    {
      glyph->outline.flags &= ~ft_outline_single_pass;

      /* copy outline to our glyph slot */
      FT_GlyphLoader_Copy_Points( glyph->internal->loader, loader->gloader );
      glyph->outline = glyph->internal->loader->base.outline;

      /* translate array so that (0,0) is the glyph's origin */
      FT_Outline_Translate( &glyph->outline, -loader->pp1.x, 0 );

      FT_Outline_Get_CBox( &glyph->outline, &bbox );

      if ( IS_HINTED( loader->load_flags ) )
      {
        /* grid-fit the bounding box */
        bbox.xMin &= -64;
        bbox.yMin &= -64;
        bbox.xMax  = ( bbox.xMax + 63 ) & -64;
        bbox.yMax  = ( bbox.yMax + 63 ) & -64;
      }
    }
    else
      bbox = loader->bbox;

    /* get the device-independent horizontal advance.  It is scaled later */
    /* by the base layer.                                                 */
    {
      FT_Pos  advance = loader->linear;


      /* the flag FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH was introduced to */
      /* correctly support DynaLab fonts, which have an incorrect       */
      /* `advance_Width_Max' field!  It is used, to my knowledge,       */
      /* exclusively in the X-TrueType font server.                     */
      /*                                                                */
      if ( face->postscript.isFixedPitch                                    &&
           ( loader->load_flags & FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH ) == 0 )
        advance = face->horizontal.advance_Width_Max;

      /* we need to return the advance in font units in linearHoriAdvance, */
      /* it will be scaled later by the base layer.                        */
      glyph->linearHoriAdvance = advance;
    }

    glyph->metrics.horiBearingX = bbox.xMin;
    glyph->metrics.horiBearingY = bbox.yMax;
    glyph->metrics.horiAdvance  = loader->pp2.x - loader->pp1.x;

    /* don't forget to hint the advance when we need to */
    if ( IS_HINTED( loader->load_flags ) )
      glyph->metrics.horiAdvance = ( glyph->metrics.horiAdvance + 32 ) & -64;

    /* Now take care of vertical metrics.  In the case where there is    */
    /* no vertical information within the font (relatively common), make */
    /* up some metrics by `hand'...                                      */

    {
      FT_Short   top_bearing;    /* vertical top side bearing (EM units) */
      FT_UShort  advance_height; /* vertical advance height   (EM units) */

      FT_Pos  left;     /* scaled vertical left side bearing         */
      FT_Pos  top;      /* scaled vertical top side bearing          */
      FT_Pos  advance;  /* scaled vertical advance height            */


      /* Get the unscaled `tsb' and `ah' */
      if ( face->vertical_info                   &&
           face->vertical.number_Of_VMetrics > 0 )
      {
        /* Don't assume that both the vertical header and vertical */
        /* metrics are present in the same font :-)                */

        TT_Get_Metrics( (TT_HoriHeader*)&face->vertical,
                        glyph_index,
                        &top_bearing,
                        &advance_height );
      }
      else
      {
        /* Make up the distances from the horizontal header.   */

        /* NOTE: The OS/2 values are the only `portable' ones, */
        /*       which is why we use them, if there is an OS/2 */
        /*       table in the font.  Otherwise, we use the     */
        /*       values defined in the horizontal header.      */
        /*                                                     */
        /* NOTE2: The sTypoDescender is negative, which is why */
        /*        we compute the baseline-to-baseline distance */
        /*        here with:                                   */
        /*             ascender - descender + linegap          */
        /*                                                     */
        if ( face->os2.version != 0xFFFF )
        {
          top_bearing    = (FT_Short)( face->os2.sTypoLineGap / 2 );
          advance_height = (FT_UShort)( face->os2.sTypoAscender -
                                        face->os2.sTypoDescender +
                                        face->os2.sTypoLineGap );
        }
        else
        {
          top_bearing    = (FT_Short)( face->horizontal.Line_Gap / 2 );
          advance_height = (FT_UShort)( face->horizontal.Ascender  +
                                        face->horizontal.Descender +
                                        face->horizontal.Line_Gap );
        }
      }

      /* We must adjust the top_bearing value from the bounding box given */
      /* in the glyph header to te bounding box calculated with           */
      /* FT_Get_Outline_CBox().                                           */

      /* scale the metrics */
      if ( !( loader->load_flags & FT_LOAD_NO_SCALE ) )
      {
        top     = FT_MulFix( top_bearing + loader->bbox.yMax, y_scale )
                    - bbox.yMax;
        advance = FT_MulFix( advance_height, y_scale );
      }
      else
      {
        top     = top_bearing + loader->bbox.yMax - bbox.yMax;
        advance = advance_height;
      }

      /* set the advance height in design units.  It is scaled later by */
      /* the base layer.                                                */
      glyph->linearVertAdvance = advance_height;

      /* XXX: for now, we have no better algorithm for the lsb, but it */
      /*      should work fine.                                        */
      /*                                                               */
      left = ( bbox.xMin - bbox.xMax ) / 2;

      /* grid-fit them if necessary */
      if ( IS_HINTED( loader->load_flags ) )
      {
        left   &= -64;
        top     = ( top + 63     ) & -64;
        advance = ( advance + 32 ) & -64;
      }

      glyph->metrics.vertBearingX = left;
      glyph->metrics.vertBearingY = top;
      glyph->metrics.vertAdvance  = advance;
    }

    /* adjust advance width to the value contained in the hdmx table */
    if ( !face->postscript.isFixedPitch && size &&
         IS_HINTED( loader->load_flags )        )
    {
      FT_Byte* widths = Get_Advance_Widths( face,
                                            size->root.metrics.x_ppem );


      if ( widths )
        glyph->metrics.horiAdvance = widths[glyph_index] << 6;
    }

    /* set glyph dimensions */
    glyph->metrics.width  = bbox.xMax - bbox.xMin;
    glyph->metrics.height = bbox.yMax - bbox.yMin;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph       :: A handle to a target slot object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled/loaded.                              */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_Glyph( TT_Size       size,
                 TT_GlyphSlot  glyph,
                 FT_UShort     glyph_index,
                 FT_UInt       load_flags )
  {
    SFNT_Interface*  sfnt;
    TT_Face          face;
    FT_Stream        stream;
    FT_Error         error;
    TT_Loader        loader;


    face   = (TT_Face)glyph->face;
    sfnt   = (SFNT_Interface*)face->sfnt;
    stream = face->root.stream;
    error  = 0;

    if ( !size || ( load_flags & FT_LOAD_NO_SCALE )   ||
                  ( load_flags & FT_LOAD_NO_RECURSE ) )
    {
      size        = NULL;
      load_flags |= FT_LOAD_NO_SCALE   |
                    FT_LOAD_NO_HINTING |
                    FT_LOAD_NO_BITMAP;
    }

    glyph->num_subglyphs = 0;

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* try to load embedded bitmap if any              */
    /*                                                 */
    /* XXX: The convention should be emphasized in     */
    /*      the documents because it can be confusing. */
    if ( size                                    &&
         size->strike_index != 0xFFFF            &&
         sfnt->load_sbits                        &&
         ( load_flags & FT_LOAD_NO_BITMAP ) == 0 )

    {
      TT_SBit_Metrics  metrics;


      error = sfnt->load_sbit_image( face,
                                     size->strike_index,
                                     glyph_index,
                                     load_flags,
                                     stream,
                                     &glyph->bitmap,
                                     &metrics );
      if ( !error )
      {
        glyph->outline.n_points   = 0;
        glyph->outline.n_contours = 0;

        glyph->metrics.width  = (FT_Pos)metrics.width  << 6;
        glyph->metrics.height = (FT_Pos)metrics.height << 6;

        glyph->metrics.horiBearingX = (FT_Pos)metrics.horiBearingX << 6;
        glyph->metrics.horiBearingY = (FT_Pos)metrics.horiBearingY << 6;
        glyph->metrics.horiAdvance  = (FT_Pos)metrics.horiAdvance  << 6;

        glyph->metrics.vertBearingX = (FT_Pos)metrics.vertBearingX << 6;
        glyph->metrics.vertBearingY = (FT_Pos)metrics.vertBearingY << 6;
        glyph->metrics.vertAdvance  = (FT_Pos)metrics.vertAdvance  << 6;

        glyph->format = ft_glyph_format_bitmap;
        if ( load_flags & FT_LOAD_VERTICAL_LAYOUT )
        {
          glyph->bitmap_left = metrics.vertBearingX;
          glyph->bitmap_top  = metrics.vertBearingY;
        }
        else
        {
          glyph->bitmap_left = metrics.horiBearingX;
          glyph->bitmap_top  = metrics.horiBearingY;
        }
        return error;
      }
    }

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* return immediately if we only want the embedded bitmaps */
    if ( load_flags & FT_LOAD_SBITS_ONLY )
      return FT_Err_Invalid_Argument;

    /* seek to the beginning of the glyph table.  For Type 42 fonts      */
    /* the table might be accessed from a Postscript stream or something */
    /* else...                                                           */

    error = face->goto_table( face, TTAG_glyf, stream, 0 );
    if ( error )
    {
      FT_ERROR(( "TT_Load_Glyph: could not access glyph table\n" ));
      goto Exit;
    }

    MEM_Set( &loader, 0, sizeof ( loader ) );

    /* update the glyph zone bounds */
    {
      FT_GlyphLoader*  gloader = FT_FACE_DRIVER(face)->glyph_loader;


      loader.gloader = gloader;

      FT_GlyphLoader_Rewind( gloader );

      tt_prepare_zone( &loader.zone, &gloader->base, 0, 0 );
      tt_prepare_zone( &loader.base, &gloader->base, 0, 0 );
    }

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( size )
    {
      /* query new execution context */
      loader.exec = size->debug ? size->context : TT_New_Context( face );
      if ( !loader.exec )
        return TT_Err_Could_Not_Find_Context;

      TT_Load_Context( loader.exec, face, size );
      loader.instructions = loader.exec->glyphIns;

      /* load default graphics state - if needed */
      if ( size->GS.instruct_control & 2 )
        loader.exec->GS = tt_default_graphics_state;
    }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* clear all outline flags, except the `owner' one */
    glyph->outline.flags = 0;

    if ( size && size->root.metrics.y_ppem < 24 )
      glyph->outline.flags |= ft_outline_high_precision;

    /* let's initialize the rest of our loader now */

    loader.load_flags    = load_flags;

    loader.face   = (FT_Face)face;
    loader.size   = (FT_Size)size;
    loader.glyph  = (FT_GlyphSlot)glyph;
    loader.stream = stream;

    loader.glyf_offset  = FILE_Pos();

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    /* if the cvt program has disabled hinting, the argument */
    /* is ignored.                                           */
    if ( size && ( size->GS.instruct_control & 1 ) )
      loader.load_flags |= FT_LOAD_NO_HINTING;

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    /* Main loading loop */
    glyph->format        = ft_glyph_format_outline;
    glyph->num_subglyphs = 0;
    error = load_truetype_glyph( &loader, glyph_index );
    if ( !error )
      compute_glyph_metrics( &loader, glyph_index );

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    if ( !size || !size->debug )
      TT_Done_Context( loader.exec );

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

  Exit:
    return error;
  }


/* END */
