/***************************************************************************/
/*                                                                         */
/*  cffgload.c                                                             */
/*                                                                         */
/*    OpenType Glyph Loader (body).                                        */
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
#include FT_OUTLINE_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H

#include "cffobjs.h"
#include "cffload.h"
#include "cffgload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffgload


  typedef enum  CFF_Operator_
  {
    cff_op_unknown = 0,

    cff_op_rmoveto,
    cff_op_hmoveto,
    cff_op_vmoveto,

    cff_op_rlineto,
    cff_op_hlineto,
    cff_op_vlineto,

    cff_op_rrcurveto,
    cff_op_hhcurveto,
    cff_op_hvcurveto,
    cff_op_rcurveline,
    cff_op_rlinecurve,
    cff_op_vhcurveto,
    cff_op_vvcurveto,

    cff_op_flex,
    cff_op_hflex,
    cff_op_hflex1,
    cff_op_flex1,

    cff_op_endchar,

    cff_op_hstem,
    cff_op_vstem,
    cff_op_hstemhm,
    cff_op_vstemhm,

    cff_op_hintmask,
    cff_op_cntrmask,
    cff_op_dotsection,  /* deprecated, acts as no-op */

    cff_op_abs,
    cff_op_add,
    cff_op_sub,
    cff_op_div,
    cff_op_neg,
    cff_op_random,
    cff_op_mul,
    cff_op_sqrt,

    cff_op_blend,

    cff_op_drop,
    cff_op_exch,
    cff_op_index,
    cff_op_roll,
    cff_op_dup,

    cff_op_put,
    cff_op_get,
    cff_op_store,
    cff_op_load,

    cff_op_and,
    cff_op_or,
    cff_op_not,
    cff_op_eq,
    cff_op_ifelse,

    cff_op_callsubr,
    cff_op_callgsubr,
    cff_op_return,

    /* do not remove */
    cff_op_max

  } CFF_Operator;


#define CFF_COUNT_CHECK_WIDTH  0x80
#define CFF_COUNT_EXACT        0x40
#define CFF_COUNT_CLEAR_STACK  0x20


  static const FT_Byte  cff_argument_counts[] =
  {
    0,  /* unknown */

    2 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT, /* rmoveto */
    1 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT,
    1 | CFF_COUNT_CHECK_WIDTH | CFF_COUNT_EXACT,

    0 | CFF_COUNT_CLEAR_STACK, /* rlineto */
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,

    0 | CFF_COUNT_CLEAR_STACK, /* rrcurveto */
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,
    0 | CFF_COUNT_CLEAR_STACK,

    13, /* flex */
    7,
    9,
    11,

    0 | CFF_COUNT_CHECK_WIDTH, /* endchar */

    2 | CFF_COUNT_CHECK_WIDTH, /* hstem */
    2 | CFF_COUNT_CHECK_WIDTH,
    2 | CFF_COUNT_CHECK_WIDTH,
    2 | CFF_COUNT_CHECK_WIDTH,

    0 | CFF_COUNT_CHECK_WIDTH, /* hintmask */
    0 | CFF_COUNT_CHECK_WIDTH, /* cntrmask */
    0, /* dotsection */

    1, /* abs */
    2,
    2,
    2,
    1,
    0,
    2,
    1,

    1, /* blend */

    1, /* drop */
    2,
    1,
    2,
    1,

    2, /* put */
    1,
    4,
    3,

    2, /* and */
    2,
    1,
    2,
    4,

    1, /* callsubr */
    1,
    0
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********                                                      *********/
  /**********             GENERIC CHARSTRING PARSING               *********/
  /**********                                                      *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Builder_Init                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph builder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    builder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    glyph   :: The current glyph object.                               */
  /*                                                                       */
  static void
  CFF_Builder_Init( CFF_Builder*   builder,
                    TT_Face        face,
                    CFF_Size       size,
                    CFF_GlyphSlot  glyph,
                    FT_Bool        hinting )
  {
    builder->path_begun  = 0;
    builder->load_points = 1;

    builder->face   = face;
    builder->glyph  = glyph;
    builder->memory = face->root.memory;

    if ( glyph )
    {
      FT_GlyphLoader*  loader = glyph->root.internal->loader;


      builder->loader  = loader;
      builder->base    = &loader->base.outline;
      builder->current = &loader->current.outline;
      FT_GlyphLoader_Rewind( loader );

      builder->hints_globals = 0;
      builder->hints_funcs   = 0;
            
      if ( hinting && size )
      {
        builder->hints_globals = size->internal;
        builder->hints_funcs   = glyph->root.internal->glyph_hints;
      }
    }

    if ( size )
    {
      builder->scale_x = size->metrics.x_scale;
      builder->scale_y = size->metrics.y_scale;
    }

    builder->pos_x = 0;
    builder->pos_y = 0;

    builder->left_bearing.x = 0;
    builder->left_bearing.y = 0;
    builder->advance.x      = 0;
    builder->advance.y      = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Builder_Done                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given glyph builder.  Its contents can still be used   */
  /*    after the call, but the function saves important information       */
  /*    within the corresponding glyph slot.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    builder :: A pointer to the glyph builder to finalize.             */
  /*                                                                       */
  static void
  CFF_Builder_Done( CFF_Builder*  builder )
  {
    CFF_GlyphSlot  glyph = builder->glyph;


    if ( glyph )
      glyph->root.outline = *builder->base;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_compute_bias                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the bias value in dependence of the number of glyph       */
  /*    subroutines.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_subrs :: The number of glyph subroutines.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The bias value.                                                    */
  static FT_Int
  cff_compute_bias( FT_UInt  num_subrs )
  {
    FT_Int  result;


    if ( num_subrs < 1240 )
      result = 107;
    else if ( num_subrs < 33900U )
      result = 1131;
    else
      result = 32768U;

    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Init_Decoder                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given glyph decoder.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder :: A pointer to the glyph builder to initialize.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face    :: The current face object.                                */
  /*                                                                       */
  /*    size    :: The current size object.                                */
  /*                                                                       */
  /*    slot    :: The current glyph object.                               */
  /*                                                                       */
  FT_LOCAL_DEF void
  CFF_Init_Decoder( CFF_Decoder*   decoder,
                    TT_Face        face,
                    CFF_Size       size,
                    CFF_GlyphSlot  slot,
                    FT_Bool        hinting )
  {
    CFF_Font*  cff = (CFF_Font*)face->extra.data;


    /* clear everything */
    MEM_Set( decoder, 0, sizeof ( *decoder ) );

    /* initialize builder */
    CFF_Builder_Init( &decoder->builder, face, size, slot, hinting );

    /* initialize Type2 decoder */
    decoder->num_globals  = cff->num_global_subrs;
    decoder->globals      = cff->global_subrs;
    decoder->globals_bias = cff_compute_bias( decoder->num_globals );
  }


  /* this function is used to select the locals subrs array */
  FT_LOCAL_DEF void
  CFF_Prepare_Decoder( CFF_Decoder*  decoder,
                       FT_UInt       glyph_index )
  {
    CFF_Font*     cff = (CFF_Font*)decoder->builder.face->extra.data;
    CFF_SubFont*  sub = &cff->top_font;


    /* manage CID fonts */
    if ( cff->num_subfonts >= 1 )
    {
      FT_Byte  fd_index = CFF_Get_FD( &cff->fd_select, glyph_index );


      sub = cff->subfonts[fd_index];
    }

    decoder->num_locals    = sub->num_local_subrs;
    decoder->locals        = sub->local_subrs;
    decoder->locals_bias   = cff_compute_bias( decoder->num_locals );

    decoder->glyph_width   = sub->private_dict.default_width;
    decoder->nominal_width = sub->private_dict.nominal_width;
  }


  /* check that there is enough room for `count' more points */
  static FT_Error
  check_points( CFF_Builder*  builder,
                FT_Int        count )
  {
    return FT_GlyphLoader_Check_Points( builder->loader, count, 0 );
  }


  /* add a new point, do not check space */
  static void
  add_point( CFF_Builder*  builder,
             FT_Pos        x,
             FT_Pos        y,
             FT_Byte       flag )
  {
    FT_Outline*  outline = builder->current;


    if ( builder->load_points )
    {
      FT_Vector*  point   = outline->points + outline->n_points;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points;


      point->x = x >> 16;
      point->y = y >> 16;
      *control = (FT_Byte)( flag ? FT_Curve_Tag_On : FT_Curve_Tag_Cubic );

      builder->last = *point;
    }
    outline->n_points++;
  }


  /* check space for a new on-curve point, then add it */
  static FT_Error
  add_point1( CFF_Builder*  builder,
              FT_Pos        x,
              FT_Pos        y )
  {
    FT_Error  error;


    error = check_points( builder, 1 );
    if ( !error )
      add_point( builder, x, y, 1 );

    return error;
  }


  /* check room for a new contour, then add it */
  static FT_Error
  add_contour( CFF_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;
    FT_Error     error;


    if ( !builder->load_points )
    {
      outline->n_contours++;
      return CFF_Err_Ok;
    }

    error = FT_GlyphLoader_Check_Points( builder->loader, 0, 1 );
    if ( !error )
    {
      if ( outline->n_contours > 0 )
        outline->contours[outline->n_contours - 1] =
          (short)( outline->n_points - 1 );

      outline->n_contours++;
    }

    return error;
  }


  /* if a path was begun, add its first on-curve point */
  static FT_Error
  start_point( CFF_Builder*  builder,
               FT_Pos        x,
               FT_Pos        y )
  {
    FT_Error  error = 0;


    /* test whether we are building a new contour */
    if ( !builder->path_begun )
    {
      builder->path_begun = 1;
      error = add_contour( builder );
      if ( !error )
        error = add_point1( builder, x, y );
    }
    return error;
  }


  /* close the current contour */
  static void
  close_contour( CFF_Builder*  builder )
  {
    FT_Outline*  outline = builder->current;

    /* XXXX: We must not include the last point in the path if it */
    /*       is located on the first point.                       */
    if ( outline->n_points > 1 )
    {
      FT_Int      first   = 0;
      FT_Vector*  p1      = outline->points + first;
      FT_Vector*  p2      = outline->points + outline->n_points - 1;
      FT_Byte*    control = (FT_Byte*)outline->tags + outline->n_points - 1;


      if ( outline->n_contours > 1 )
      {
        first = outline->contours[outline->n_contours - 2] + 1;
        p1    = outline->points + first;
      }

      /* `delete' last point only if it coincides with the first */
      /* point and it is not a control point (which can happen). */
      if ( p1->x == p2->x && p1->y == p2->y )
        if ( *control == FT_Curve_Tag_On )
          outline->n_points--;
    }

    if ( outline->n_contours > 0 )
      outline->contours[outline->n_contours - 1] =
        (short)( outline->n_points - 1 );
  }


  static FT_Int
  cff_lookup_glyph_by_stdcharcode( CFF_Font*  cff,
                                   FT_Int     charcode )
  {
    FT_UInt    n;
    FT_UShort  glyph_sid;


    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    /* Get code to SID mapping from `cff_standard_encoding'. */
    glyph_sid = CFF_Get_Standard_Encoding( (FT_UInt)charcode );

    for ( n = 0; n < cff->num_glyphs; n++ )
    {
      if ( cff->charset.sids[n] == glyph_sid )
        return n;
    }

    return -1;
  }


  static FT_Error
  cff_operator_seac( CFF_Decoder*  decoder,
                     FT_Pos        adx,
                     FT_Pos        ady,
                     FT_Int        bchar,
                     FT_Int        achar )
  {
    FT_Error     error;
    FT_Int       bchar_index, achar_index, n_base_points;
    FT_Outline*  base = decoder->builder.base;
    TT_Face      face = decoder->builder.face;
    CFF_Font*    cff  = (CFF_Font*)(face->extra.data);
    FT_Vector    left_bearing, advance;
    FT_Byte*     charstring;
    FT_ULong     charstring_len;


    bchar_index = cff_lookup_glyph_by_stdcharcode( cff, bchar );
    achar_index = cff_lookup_glyph_by_stdcharcode( cff, achar );

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "cff_operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return CFF_Err_Syntax_Error;
    }

    /* If we are trying to load a composite glyph, do not load the */
    /* accent character and return the array of subglyphs.         */
    if ( decoder->builder.no_recurse )
    {
      FT_GlyphSlot     glyph  = (FT_GlyphSlot)decoder->builder.glyph;
      FT_GlyphLoader*  loader = glyph->internal->loader;
      FT_SubGlyph*     subg;


      /* reallocate subglyph array if necessary */
      error = FT_GlyphLoader_Check_Subglyphs( loader, 2 );
      if ( error )
        goto Exit;

      subg = loader->current.subglyphs;

      /* subglyph 0 = base character */
      subg->index = bchar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES |
                    FT_SUBGLYPH_FLAG_USE_MY_METRICS;
      subg->arg1  = 0;
      subg->arg2  = 0;
      subg++;

      /* subglyph 1 = accent character */
      subg->index = achar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES;
      subg->arg1  = adx;
      subg->arg2  = ady;

      /* set up remaining glyph fields */
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = ft_glyph_format_composite;

      loader->current.num_subglyphs = 2;
    }

    /* First load `bchar' in builder */
    error = CFF_Access_Element( &cff->charstrings_index, bchar_index,
                                &charstring, &charstring_len );
    if ( !error )
    {
      error = CFF_Parse_CharStrings( decoder, charstring, charstring_len );

      if ( error )
        goto Exit;

      CFF_Forget_Element( &cff->charstrings_index, &charstring );
    }

    n_base_points = base->n_points;

    /* Save the left bearing and width of the base character */
    /* as they will be erased by the next load.              */

    left_bearing = decoder->builder.left_bearing;
    advance      = decoder->builder.advance;

    decoder->builder.left_bearing.x = 0;
    decoder->builder.left_bearing.y = 0;

    /* Now load `achar' on top of the base outline. */
    error = CFF_Access_Element( &cff->charstrings_index, achar_index,
                                &charstring, &charstring_len );
    if ( !error )
    {
      error = CFF_Parse_CharStrings( decoder, charstring, charstring_len );

      if ( error )
        goto Exit;

      CFF_Forget_Element( &cff->charstrings_index, &charstring );
    }

    /* Restore the left side bearing and advance width */
    /* of the base character.                          */
    decoder->builder.left_bearing = left_bearing;
    decoder->builder.advance      = advance;

    /* Finally, move the accent. */
    if ( decoder->builder.load_points )
    {
      FT_Outline  dummy;


      dummy.n_points = (short)( base->n_points - n_base_points );
      dummy.points   = base->points   + n_base_points;

      FT_Outline_Translate( &dummy, adx, ady );
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Parse_CharStrings                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 2 charstrings program.                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charstring_base :: The base of the charstring stream.              */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  CFF_Parse_CharStrings( CFF_Decoder*  decoder,
                         FT_Byte*      charstring_base,
                         FT_Int        charstring_len )
  {
    FT_Error           error;
    CFF_Decoder_Zone*  zone;
    FT_Byte*           ip;
    FT_Byte*           limit;
    CFF_Builder*       builder = &decoder->builder;
    FT_Pos             x, y;
    FT_Fixed           seed;
    FT_Fixed*          stack;

    T2_Hints_Funcs     hinter;


    /* set default width */
    decoder->num_hints  = 0;
    decoder->read_width = 1;

    /* compute random seed from stack address of parameter */
    seed = (FT_Fixed)(char*)&seed           ^
           (FT_Fixed)(char*)&decoder        ^
           (FT_Fixed)(char*)&charstring_base;
    seed = ( seed ^ ( seed >> 10 ) ^ ( seed >> 20 ) ) & 0xFFFF;
    if ( seed == 0 )
      seed = 0x7384;

    /* initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;
    stack         = decoder->top;

    hinter = (T2_Hints_Funcs) builder->hints_funcs;

    builder->path_begun = 0;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = CFF_Err_Ok;

    x = builder->pos_x;
    y = builder->pos_y;

    /* begin hints recording session, if any */
    if ( hinter )
      hinter->open( hinter->hints );

    /* now, execute loop */
    while ( ip < limit )
    {
      CFF_Operator  op;
      FT_Byte       v;


      /********************************************************************/
      /*                                                                  */
      /* Decode operator or operand                                       */
      /*                                                                  */
      v = *ip++;
      if ( v >= 32 || v == 28 )
      {
        FT_Int    shift = 16;
        FT_Int32  val;


        /* this is an operand, push it on the stack */
        if ( v == 28 )
        {
          if ( ip + 1 >= limit )
            goto Syntax_Error;
          val = (FT_Short)( ( (FT_Short)ip[0] << 8 ) | ip[1] );
          ip += 2;
        }
        else if ( v < 247 )
          val = (FT_Long)v - 139;
        else if ( v < 251 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = ( (FT_Long)v - 247 ) * 256 + *ip++ + 108;
        }
        else if ( v < 255 )
        {
          if ( ip >= limit )
            goto Syntax_Error;
          val = -( (FT_Long)v - 251 ) * 256 - *ip++ - 108;
        }
        else
        {
          if ( ip + 3 >= limit )
            goto Syntax_Error;
          val = ( (FT_Int32)ip[0] << 24 ) |
                ( (FT_Int32)ip[1] << 16 ) |
                ( (FT_Int32)ip[2] <<  8 ) |
                            ip[3];
          ip    += 4;
          shift  = 0;
        }
        if ( decoder->top - stack >= CFF_MAX_OPERANDS )
          goto Stack_Overflow;

        val           <<= shift;
        *decoder->top++ = val;

#ifdef FT_DEBUG_LEVEL_TRACE
        if ( !( val & 0xFFFF ) )
          FT_TRACE4(( " %d", (FT_Int32)( val >> 16 ) ));
        else
          FT_TRACE4(( " %.2f", val / 65536.0 ));
#endif

      }
      else
      {
        FT_Fixed*  args     = decoder->top;
        FT_Int     num_args = args - decoder->stack;
        FT_Int     req_args;


        /* find operator */
        op = cff_op_unknown;

        switch ( v )
        {
        case 1:
          op = cff_op_hstem;
          break;
        case 3:
          op = cff_op_vstem;
          break;
        case 4:
          op = cff_op_vmoveto;
          break;
        case 5:
          op = cff_op_rlineto;
          break;
        case 6:
          op = cff_op_hlineto;
          break;
        case 7:
          op = cff_op_vlineto;
          break;
        case 8:
          op = cff_op_rrcurveto;
          break;
        case 10:
          op = cff_op_callsubr;
          break;
        case 11:
          op = cff_op_return;
          break;
        case 12:
          {
            if ( ip >= limit )
              goto Syntax_Error;
            v = *ip++;

            switch ( v )
            {
            case 0:
              op = cff_op_dotsection;
              break;
            case 3:
              op = cff_op_and;
              break;
            case 4:
              op = cff_op_or;
              break;
            case 5:
              op = cff_op_not;
              break;
            case 8:
              op = cff_op_store;
              break;
            case 9:
              op = cff_op_abs;
              break;
            case 10:
              op = cff_op_add;
              break;
            case 11:
              op = cff_op_sub;
              break;
            case 12:
              op = cff_op_div;
              break;
            case 13:
              op = cff_op_load;
              break;
            case 14:
              op = cff_op_neg;
              break;
            case 15:
              op = cff_op_eq;
              break;
            case 18:
              op = cff_op_drop;
              break;
            case 20:
              op = cff_op_put;
              break;
            case 21:
              op = cff_op_get;
              break;
            case 22:
              op = cff_op_ifelse;
              break;
            case 23:
              op = cff_op_random;
              break;
            case 24:
              op = cff_op_mul;
              break;
            case 26:
              op = cff_op_sqrt;
              break;
            case 27:
              op = cff_op_dup;
              break;
            case 28:
              op = cff_op_exch;
              break;
            case 29:
              op = cff_op_index;
              break;
            case 30:
              op = cff_op_roll;
              break;
            case 34:
              op = cff_op_hflex;
              break;
            case 35:
              op = cff_op_flex;
              break;
            case 36:
              op = cff_op_hflex1;
              break;
            case 37:
              op = cff_op_flex1;
              break;
            default:
              /* decrement ip for syntax error message */
              ip--;
            }
          }
          break;
        case 14:
          op = cff_op_endchar;
          break;
        case 16:
          op = cff_op_blend;
          break;
        case 18:
          op = cff_op_hstemhm;
          break;
        case 19:
          op = cff_op_hintmask;
          break;
        case 20:
          op = cff_op_cntrmask;
          break;
        case 21:
          op = cff_op_rmoveto;
          break;
        case 22:
          op = cff_op_hmoveto;
          break;
        case 23:
          op = cff_op_vstemhm;
          break;
        case 24:
          op = cff_op_rcurveline;
          break;
        case 25:
          op = cff_op_rlinecurve;
          break;
        case 26:
          op = cff_op_vvcurveto;
          break;
        case 27:
          op = cff_op_hhcurveto;
          break;
        case 29:
          op = cff_op_callgsubr;
          break;
        case 30:
          op = cff_op_vhcurveto;
          break;
        case 31:
          op = cff_op_hvcurveto;
          break;
        default:
          ;
        }
        if ( op == cff_op_unknown )
          goto Syntax_Error;

        /* check arguments */
        req_args = cff_argument_counts[op];
        if ( req_args & CFF_COUNT_CHECK_WIDTH )
        {
          args = stack;

          if ( num_args > 0 && decoder->read_width )
          {
            /* If `nominal_width' is non-zero, the number is really a      */
            /* difference against `nominal_width'.  Else, the number here  */
            /* is truly a width, not a difference against `nominal_width'. */
            /* If the font does not set `nominal_width', then              */
            /* `nominal_width' defaults to zero, and so we can set         */
            /* `glyph_width' to `nominal_width' plus number on the stack   */
            /* -- for either case.                                         */

            FT_Int set_width_ok;


            switch ( op )
            {
            case cff_op_hmoveto:
            case cff_op_vmoveto:
              set_width_ok = num_args & 2;
              break;

            case cff_op_hstem:
            case cff_op_vstem:
            case cff_op_hstemhm:
            case cff_op_vstemhm:
            case cff_op_rmoveto:
              set_width_ok = num_args & 1;
              break;

            case cff_op_endchar:
              /* If there is a width specified for endchar, we either have */
              /* 1 argument or 5 arguments.  We like to argue.             */
              set_width_ok = ( ( num_args == 5 ) || ( num_args == 1 ) );
              break;

            default:
              set_width_ok = 0;
              break;
            }

            if ( set_width_ok )
            {
              decoder->glyph_width = decoder->nominal_width +
                                       ( stack[0] >> 16 );

              /* Consumed an argument. */
              num_args--;
              args++;
            }
          }

          decoder->read_width = 0;
          req_args            = 0;
        }

        req_args &= 15;
        if ( num_args < req_args )
          goto Stack_Underflow;
        args     -= req_args;
        num_args -= req_args;

        switch ( op )
        {
        case cff_op_hstem:
        case cff_op_vstem:
        case cff_op_hstemhm:
        case cff_op_vstemhm:
          /* the number of arguments is always even here */
          FT_TRACE4(( op == cff_op_hstem   ? " hstem"   :
                    ( op == cff_op_vstem   ? " vstem"   :
                    ( op == cff_op_hstemhm ? " hstemhm" : " vstemhm" ) ) ));

          if ( hinter )
            hinter->stems( hinter->hints,
                           ( op == cff_op_hstem || op == cff_op_hstemhm ),
                           num_args / 2,
                           args );

          decoder->num_hints += num_args / 2;
          args = stack;
          break;

        case cff_op_hintmask:
        case cff_op_cntrmask:
          FT_TRACE4(( op == cff_op_hintmask ? " hintmask" : " cntrmask" ));
  
          /* implement vstem when needed --                        */
          /* the specification doesn't say it, but this also works */
          /* with the 'cntrmask' operator                          */
          /*                                                       */
          if ( num_args > 0 )
          {
            if ( hinter )
              hinter->stems( hinter->hints,
                             0,
                             num_args / 2,
                             args );
          
            decoder->num_hints += num_args / 2;
          }

          if ( hinter )
          {
            if ( op == cff_op_hintmask )
              hinter->hintmask( hinter->hints,
                                builder->current->n_points,
                                decoder->num_hints,
                                ip );
            else
              hinter->counter( hinter->hints,
                               decoder->num_hints,
                               ip );
          }

#ifdef FT_DEBUG_LEVEL_TRACE
          {
            FT_UInt maskbyte;

            FT_TRACE4(( " " ));

            for ( maskbyte = 0;
                  maskbyte < (FT_UInt)(( decoder->num_hints + 7 ) >> 3);
                  maskbyte++, ip++ )
            {
              FT_TRACE4(( "%02X", *ip ));
            }

          }
#else
          ip += ( decoder->num_hints + 7 ) >> 3;
#endif
          if ( ip >= limit )
            goto Syntax_Error;
          args = stack;
          break;

        case cff_op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          y   += args[1];
          args = stack;
          break;

        case cff_op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          y   += args[0];
          args = stack;
          break;

        case cff_op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          close_contour( builder );
          builder->path_begun = 0;
          x   += args[0];
          args = stack;
          break;

        case cff_op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point ( builder, x, y )         ||
               check_points( builder, num_args / 2 ) )
            goto Memory_Error;

          if ( num_args < 2 || num_args & 1 )
            goto Stack_Underflow;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 1 );
            args += 2;
          }
          args = stack;
          break;

        case cff_op_hlineto:
        case cff_op_vlineto:
          {
            FT_Int  phase = ( op == cff_op_hlineto );


            FT_TRACE4(( op == cff_op_hlineto ? " hlineto"
                                             : " vlineto" ));

            if ( start_point ( builder, x, y )     ||
                 check_points( builder, num_args ) )
              goto Memory_Error;

            args = stack;
            while (args < decoder->top )
            {
              if ( phase )
                x += args[0];
              else
                y += args[0];

              if ( add_point1( builder, x, y ) )
                goto Memory_Error;

              args++;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case cff_op_rrcurveto:
          FT_TRACE4(( " rrcurveto" ));

          /* check number of arguments; must be a multiple of 6 */
          if ( num_args % 6 != 0 )
            goto Stack_Underflow;

          if ( start_point ( builder, x, y )         ||
               check_points( builder, num_args / 2 ) )
            goto Memory_Error;

          args = stack;
          while ( args < decoder->top )
          {
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            add_point( builder, x, y, 1 );
            args += 6;
          }
          args = stack;
          break;

        case cff_op_vvcurveto:
          FT_TRACE4(( " vvcurveto" ));

          if ( start_point ( builder, x, y ) )
            goto Memory_Error;

          args = stack;
          if ( num_args & 1 )
          {
            x += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Memory_Error;

          while ( args < decoder->top )
          {
            y += args[0];
            add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );
            y += args[3];
            add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case cff_op_hhcurveto:
          FT_TRACE4(( " hhcurveto" ));

          if ( start_point ( builder, x, y ) )
            goto Memory_Error;

          args = stack;
          if ( num_args & 1 )
          {
            y += args[0];
            args++;
            num_args--;
          }

          if ( num_args % 4 != 0 )
            goto Stack_Underflow;

          if ( check_points( builder, 3 * ( num_args / 4 ) ) )
            goto Memory_Error;

          while ( args < decoder->top )
          {
            x += args[0];
            add_point( builder, x, y, 0 );
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );
            x += args[3];
            add_point( builder, x, y, 1 );
            args += 4;
          }
          args = stack;
          break;

        case cff_op_vhcurveto:
        case cff_op_hvcurveto:
          {
            FT_Int  phase;


            FT_TRACE4(( op == cff_op_vhcurveto ? " vhcurveto"
                                               : " hvcurveto" ));

            if ( start_point ( builder, x, y ) )
              goto Memory_Error;

            args = stack;
            if (num_args < 4 || ( num_args % 4 ) > 1 )
              goto Stack_Underflow;

            if ( check_points( builder, ( num_args / 4 ) * 3 ) )
              goto Stack_Underflow;

            phase = ( op == cff_op_hvcurveto );

            while ( num_args >= 4 )
            {
              num_args -= 4;
              if ( phase )
              {
                x += args[0];
                add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                add_point( builder, x, y, 0 );
                y += args[3];
                if ( num_args == 1 )
                  x += args[4];
                add_point( builder, x, y, 1 );
              }
              else
              {
                y += args[0];
                add_point( builder, x, y, 0 );
                x += args[1];
                y += args[2];
                add_point( builder, x, y, 0 );
                x += args[3];
                if ( num_args == 1 )
                  y += args[4];
                add_point( builder, x, y, 1 );
              }
              args  += 4;
              phase ^= 1;
            }
            args = stack;
          }
          break;

        case cff_op_rlinecurve:
          {
            FT_Int  num_lines = ( num_args - 6 ) / 2;


            FT_TRACE4(( " rlinecurve" ));

            if ( num_args < 8 || ( num_args - 6 ) & 1 )
              goto Stack_Underflow;

            if ( start_point( builder, x, y )           ||
                 check_points( builder, num_lines + 3 ) )
              goto Memory_Error;

            args = stack;

            /* first, add the line segments */
            while ( num_lines > 0 )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, 1 );
              args += 2;
              num_lines--;
            }

            /* then the curve */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );
            x += args[4];
            y += args[5];
            add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case cff_op_rcurveline:
          {
            FT_Int  num_curves = ( num_args - 2 ) / 6;


            FT_TRACE4(( " rcurveline" ));

            if ( num_args < 8 || ( num_args - 2 ) % 6 )
              goto Stack_Underflow;

            if ( start_point ( builder, x, y )             ||
                 check_points( builder, num_curves*3 + 2 ) )
              goto Memory_Error;

            args = stack;

            /* first, add the curves */
            while ( num_curves > 0 )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, 0 );
              x += args[2];
              y += args[3];
              add_point( builder, x, y, 0 );
              x += args[4];
              y += args[5];
              add_point( builder, x, y, 1 );
              args += 6;
              num_curves--;
            }

            /* then the final line */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 1 );
            args = stack;
          }
          break;

        case cff_op_hflex1:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex1" ));

            args = stack;

            /* adding five more points; 4 control points, 1 on-curve point */
            /* make sure we have enough space for the start point if it    */
            /* needs to be added..                                         */
            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
              goto Memory_Error;

            /* Record the starting point's y postion for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            y += args[1];
            add_point( builder, x, y, 0 );

            /* second control point */
            x += args[2];
            y += args[3];
            add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[4];
            add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[5];
            add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[6];
            y += args[7];
            add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start   */
            x += args[8];
            y  = start_y;
            add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_hflex:
          {
            FT_Pos start_y;


            FT_TRACE4(( " hflex" ));

            args = stack;

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( start_point( builder, x, y ) ||
                 check_points ( builder, 6 )  )
              goto Memory_Error;

            /* record the starting point's y-position for later use */
            start_y = y;

            /* first control point */
            x += args[0];
            add_point( builder, x, y, 0 );

            /* second control point */
            x += args[1];
            y += args[2];
            add_point( builder, x, y, 0 );

            /* join point; on curve, with y-value the same as the last */
            /* control point's y-value                                 */
            x += args[3];
            add_point( builder, x, y, 1 );

            /* third control point, with y-value the same as the join */
            /* point's y-value                                        */
            x += args[4];
            add_point( builder, x, y, 0 );

            /* fourth control point */
            x += args[5];
            y  = start_y;
            add_point( builder, x, y, 0 );

            /* ending point, with y-value the same as the start point's */
            /* y-value -- we don't add this point, though               */
            x += args[6];
            add_point( builder, x, y, 1 );

            args = stack;
            break;
          }

        case cff_op_flex1:
          {
            FT_Pos  start_x, start_y; /* record start x, y values for alter */
                                      /* use                                */
            FT_Int  dx = 0, dy = 0;   /* used in horizontal/vertical        */
                                      /* algorithm below                    */
            FT_Int  horizontal, count;


            FT_TRACE4(( " flex1" ));

            /* adding six more points; 4 control points, 2 on-curve points */
            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
               goto Memory_Error;

            /* record the starting point's x, y postion for later use */
            start_x = x;
            start_y = y;

            /* XXX: figure out whether this is supposed to be a horizontal */
            /*      or vertical flex; the Type 2 specification is vague... */

            args = stack;

            /* grab up to the last argument */
            for ( count = 5; count > 0; count-- )
            {
              dx += args[0];
              dy += args[1];
              args += 2;
            }

            /* rewind */
            args = stack;

            if ( dx < 0 ) dx = -dx;
            if ( dy < 0 ) dy = -dy;

            /* strange test, but here it is... */
            horizontal = ( dx > dy );

            for ( count = 5; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y, (FT_Bool)( count == 3 ) );
              args += 2;
            }

            /* is last operand an x- or y-delta? */
            if ( horizontal )
            {
              x += args[0];
              y  = start_y;
            }
            else
            {
              x  = start_x;
              y += args[0];
            }

            add_point( builder, x, y, 1 );

            args = stack;
            break;
           }

        case cff_op_flex:
          {
            FT_UInt  count;


            FT_TRACE4(( " flex" ));

            if ( start_point( builder, x, y ) ||
                 check_points( builder, 6 )   )
              goto Memory_Error;

            args = stack;
            for ( count = 6; count > 0; count-- )
            {
              x += args[0];
              y += args[1];
              add_point( builder, x, y,
                         (FT_Bool)( count == 3 || count == 0 ) );
              args += 2;
            }

            args = stack;
          }
          break;

        case cff_op_endchar:
          FT_TRACE4(( " endchar" ));

          /* We are going to emulate the seac operator. */
          if ( num_args == 4 )
          {
            error = cff_operator_seac( decoder,
                                       args[0] >> 16, args[1] >> 16,
                                       args[2] >> 16, args[3] >> 16 );
            args += 4;
          }

          if ( !error )
            error = CFF_Err_Ok;

          close_contour( builder );

          /* close hints recording session */
          if ( hinter )
          {
            if (hinter->close( hinter->hints, builder->current->n_points ) )
              goto Syntax_Error;

            /* apply hints to the loaded glyph outline now */
            hinter->apply( hinter->hints,
                           builder->current,
                           (PSH_Globals)builder->hints_globals );
          }

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return error;

        case cff_op_abs:
          FT_TRACE4(( " abs" ));

          if ( args[0] < 0 )
            args[0] = -args[0];
          args++;
          break;

        case cff_op_add:
          FT_TRACE4(( " add" ));

          args[0] += args[1];
          args++;
          break;

        case cff_op_sub:
          FT_TRACE4(( " sub" ));

          args[0] -= args[1];
          args++;
          break;

        case cff_op_div:
          FT_TRACE4(( " div" ));

          args[0] = FT_DivFix( args[0], args[1] );
          args++;
          break;

        case cff_op_neg:
          FT_TRACE4(( " neg" ));

          args[0] = -args[0];
          args++;
          break;

        case cff_op_random:
          {
            FT_Fixed  rand;


            FT_TRACE4(( " rand" ));

            rand = seed;
            if ( rand >= 0x8000 )
              rand++;

            args[0] = rand;
            seed    = FT_MulFix( seed, 0x10000L - seed );
            if ( seed == 0 )
              seed += 0x2873;
            args++;
          }
          break;

        case cff_op_mul:
          FT_TRACE4(( " mul" ));

          args[0] = FT_MulFix( args[0], args[1] );
          args++;
          break;

        case cff_op_sqrt:
          FT_TRACE4(( " sqrt" ));

          if ( args[0] > 0 )
          {
            FT_Int    count = 9;
            FT_Fixed  root  = args[0];
            FT_Fixed  new_root;


            for (;;)
            {
              new_root = ( root + FT_DivFix( args[0], root ) + 1 ) >> 1;
              if ( new_root == root || count <= 0 )
                break;
              root = new_root;
            }
            args[0] = new_root;
          }
          else
            args[0] = 0;
          args++;
          break;

        case cff_op_drop:
          /* nothing */
          FT_TRACE4(( " drop" ));

          break;

        case cff_op_exch:
          {
            FT_Fixed  tmp;


            FT_TRACE4(( " exch" ));

            tmp     = args[0];
            args[0] = args[1];
            args[1] = tmp;
            args   += 2;
          }
          break;

        case cff_op_index:
          {
            FT_Int  index = args[0] >> 16;


            FT_TRACE4(( " index" ));

            if ( index < 0 )
              index = 0;
            else if ( index > num_args - 2 )
              index = num_args - 2;
            args[0] = args[-( index + 1 )];
            args++;
          }
          break;

        case cff_op_roll:
          {
            FT_Int  count = (FT_Int)( args[0] >> 16 );
            FT_Int  index = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " roll" ));

            if ( count <= 0 )
              count = 1;

            args -= count;
            if ( args < stack )
              goto Stack_Underflow;

            if ( index >= 0 )
            {
              while ( index > 0 )
              {
                FT_Fixed  tmp = args[count - 1];
                FT_Int    i;


                for ( i = count - 2; i >= 0; i-- )
                  args[i + 1] = args[i];
                args[0] = tmp;
                index--;
              }
            }
            else
            {
              while ( index < 0 )
              {
                FT_Fixed  tmp = args[0];
                FT_Int    i;


                for ( i = 0; i < count - 1; i++ )
                  args[i] = args[i + 1];
                args[count - 1] = tmp;
                index++;
              }
            }
            args += count;
          }
          break;

        case cff_op_dup:
          FT_TRACE4(( " dup" ));

          args[1] = args[0];
          args++;
          break;

        case cff_op_put:
          {
            FT_Fixed  val   = args[0];
            FT_Int    index = (FT_Int)( args[1] >> 16 );


            FT_TRACE4(( " put" ));

            if ( index >= 0 && index < decoder->len_buildchar )
              decoder->buildchar[index] = val;
          }
          break;

        case cff_op_get:
          {
            FT_Int   index = (FT_Int)( args[0] >> 16 );
            FT_Fixed val   = 0;


            FT_TRACE4(( " get" ));

            if ( index >= 0 && index < decoder->len_buildchar )
              val = decoder->buildchar[index];

            args[0] = val;
            args++;
          }
          break;

        case cff_op_store:
          FT_TRACE4(( " store "));

          goto Unimplemented;

        case cff_op_load:
          FT_TRACE4(( " load" ));

          goto Unimplemented;

        case cff_op_dotsection:
          /* this operator is deprecated and ignored by the parser */
          FT_TRACE4(( " dotsection" ));
          break;

        case cff_op_and:
          {
            FT_Fixed  cond = args[0] && args[1];


            FT_TRACE4(( " and" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_or:
          {
            FT_Fixed  cond = args[0] || args[1];


            FT_TRACE4(( " or" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_eq:
          {
            FT_Fixed  cond = !args[0];


            FT_TRACE4(( " eq" ));

            args[0] = cond ? 0x10000L : 0;
            args++;
          }
          break;

        case cff_op_ifelse:
          {
            FT_Fixed  cond = (args[2] <= args[3]);


            FT_TRACE4(( " ifelse" ));

            if ( !cond )
              args[0] = args[1];
            args++;
          }
          break;

        case cff_op_callsubr:
          {
            FT_UInt  index = (FT_UInt)( ( args[0] >> 16 ) +
                                        decoder->locals_bias );


            FT_TRACE4(( " callsubr(%d)", index ));

            if ( index >= decoder->num_locals )
            {
              FT_ERROR(( "CFF_Parse_CharStrings:" ));
              FT_ERROR(( "  invalid local subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= CFF_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "CFF_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->locals[index];
            zone->limit  = decoder->locals[index + 1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "CFF_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case cff_op_callgsubr:
          {
            FT_UInt  index = (FT_UInt)( ( args[0] >> 16 ) +
                                        decoder->globals_bias );


            FT_TRACE4(( " callgsubr(%d)", index ));

            if ( index >= decoder->num_globals )
            {
              FT_ERROR(( "CFF_Parse_CharStrings:" ));
              FT_ERROR(( " invalid global subr index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= CFF_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "CFF_Parse_CharStrings: too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;
            zone->base   = decoder->globals[index];
            zone->limit  = decoder->globals[index+1];
            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "CFF_Parse_CharStrings: invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
          }
          break;

        case cff_op_return:
          FT_TRACE4(( " return" ));

          if ( decoder->zone <= decoder->zones )
          {
            FT_ERROR(( "CFF_Parse_CharStrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          decoder->zone--;
          zone  = decoder->zone;
          ip    = zone->cursor;
          limit = zone->limit;
          break;

        default:
        Unimplemented:
          FT_ERROR(( "Unimplemented opcode: %d", ip[-1] ));

          if ( ip[-1] == 12 )
            FT_ERROR(( " %d", ip[0] ));
          FT_ERROR(( "\n" ));

          return CFF_Err_Unimplemented_Feature;
        }

      decoder->top = args;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

    return error;

  Syntax_Error:
    FT_TRACE4(( "CFF_Parse_CharStrings: syntax error!" ));
    return CFF_Err_Invalid_File_Format;

  Stack_Underflow:
    FT_TRACE4(( "CFF_Parse_CharStrings: stack underflow!" ));
    return CFF_Err_Too_Few_Arguments;

  Stack_Overflow:
    FT_TRACE4(( "CFF_Parse_CharStrings: stack overflow!" ));
    return CFF_Err_Stack_Overflow;

  Memory_Error:
    return builder->error;
  }


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


#if 0 /* unused until we support pure CFF fonts */


  FT_LOCAL_DEF FT_Error
  CFF_Compute_Max_Advance( TT_Face  face,
                           FT_Int*  max_advance )
  {
    FT_Error     error = 0;
    CFF_Decoder  decoder;
    FT_Int       glyph_index;
    CFF_Font*    cff = (CFF_Font*)face->other;


    *max_advance = 0;

    /* Initialize load decoder */
    CFF_Init_Decoder( &decoder, face, 0, 0, 0 );

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    /* For each glyph, parse the glyph charstring and extract */
    /* the advance width.                                     */
    for ( glyph_index = 0; glyph_index < face->root.num_glyphs;
          glyph_index++ )
    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      /* now get load the unscaled outline */
      error = CFF_Access_Element( &cff->charstrings_index, glyph_index,
                                  &charstring, &charstring_len );
      if ( !error )
      {
        CFF_Prepare_Decoder( &decoder, glyph_index );
        error = CFF_Parse_CharStrings( &decoder, charstring, charstring_len );

        CFF_Forget_Element( &cff->charstrings_index, &charstring );
      }

      /* ignore the error if one has occurred -- skip to next glyph */
      error = 0;
    }

    *max_advance = decoder.builder.advance.x;

    return CFF_Err_Ok;
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
  CFF_Load_Glyph( CFF_GlyphSlot  glyph,
                  CFF_Size       size,
                  FT_Int         glyph_index,
                  FT_Int         load_flags )
  {
    FT_Error     error;
    CFF_Decoder  decoder;
    TT_Face      face = (TT_Face)glyph->root.face;
    FT_Bool      hinting;
    CFF_Font*    cff = (CFF_Font*)face->extra.data;

    FT_Matrix    font_matrix;
    FT_Vector    font_offset;


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = 0x10000L;
    glyph->y_scale = 0x10000L;
    if ( size )
    {
      glyph->x_scale = size->metrics.x_scale;
      glyph->y_scale = size->metrics.y_scale;
    }

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    glyph->root.format = ft_glyph_format_outline;  /* by default */

    {
      FT_Byte*  charstring;
      FT_ULong  charstring_len;


      CFF_Init_Decoder( &decoder, face, size, glyph, hinting );

      decoder.builder.no_recurse =
        (FT_Bool)( ( load_flags & FT_LOAD_NO_RECURSE ) != 0 );

      /* now load the unscaled outline */
      error = CFF_Access_Element( &cff->charstrings_index, glyph_index,
                                  &charstring, &charstring_len );
      if ( !error )
      {
        CFF_Index csindex = cff->charstrings_index;


        CFF_Prepare_Decoder( &decoder, glyph_index );
        error = CFF_Parse_CharStrings( &decoder, charstring, charstring_len );

        CFF_Forget_Element( &cff->charstrings_index, &charstring );

        /* We set control_data and control_len if charstrings is loaded.  */
        /* See how charstring loads at CFF_Access_Element() in cffload.c. */

        glyph->root.control_data =
          csindex.bytes + csindex.offsets[glyph_index] - 1;
        glyph->root.control_len =
          charstring_len;
      }

      /* save new glyph tables */
      CFF_Builder_Done( &decoder.builder );
    }

    font_matrix = cff->top_font.font_dict.font_matrix;
    font_offset = cff->top_font.font_dict.font_offset;

    /* Now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax.                                   */
    if ( !error )
    {
      /* For composite glyphs, return only left side bearing and */
      /* advance width.                                          */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_Slot_Internal  internal = glyph->root.internal;


        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.glyph_width;
        internal->glyph_matrix           = font_matrix;
        internal->glyph_delta            = font_offset;
        internal->glyph_transformed      = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance                    = decoder.glyph_width;
        glyph->root.linearHoriAdvance           = decoder.glyph_width;
        glyph->root.internal->glyph_transformed = 0;

        /* make up vertical metrics */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;

        glyph->root.linearVertAdvance = 0;

        glyph->root.format = ft_glyph_format_outline;

        glyph->root.outline.flags = 0;
        if ( size && size->metrics.y_ppem < 24 )
          glyph->root.outline.flags |= ft_outline_high_precision;

        glyph->root.outline.flags |= ft_outline_reverse_fill;

        /* apply the font matrix */
        FT_Outline_Transform( &glyph->root.outline, &font_matrix );

        FT_Outline_Translate( &glyph->root.outline,
                              font_offset.x,
                              font_offset.y );

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur     = &glyph->root.outline;
          FT_Vector*   vec     = cur->points;
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
