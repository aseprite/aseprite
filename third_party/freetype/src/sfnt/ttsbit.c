/***************************************************************************/
/*                                                                         */
/*  ttsbit.c                                                               */
/*                                                                         */
/*    TrueType and OpenType embedded bitmap support (body).                */
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
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttsbit.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttsbit


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    blit_sbit                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Blits a bitmap from an input stream into a given target.  Supports */
  /*    x and y offsets as well as byte padded lines.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    target      :: The target bitmap/pixmap.                           */
  /*                                                                       */
  /*    source      :: The input packed bitmap data.                       */
  /*                                                                       */
  /*    line_bits   :: The number of bits per line.                        */
  /*                                                                       */
  /*    byte_padded :: A flag which is true if lines are byte-padded.      */
  /*                                                                       */
  /*    x_offset    :: The horizontal offset.                              */
  /*                                                                       */
  /*    y_offset    :: The vertical offset.                                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    IMPORTANT: The x and y offsets are relative to the top corner of   */
  /*               the target bitmap (unlike the normal TrueType           */
  /*               convention).  A positive y offset indicates a downwards */
  /*               direction!                                              */
  /*                                                                       */
  static void
  blit_sbit( FT_Bitmap*  target,
             FT_Byte*    source,
             FT_Int      line_bits,
             FT_Bool     byte_padded,
             FT_Int      x_offset,
             FT_Int      y_offset )
  {
    FT_Byte*   line_buff;
    FT_Int     line_incr;
    FT_Int     height;

    FT_UShort  acc;
    FT_UInt    loaded;


    /* first of all, compute starting write position */
    line_incr = target->pitch;
    line_buff = target->buffer;

    if ( line_incr < 0 )
      line_buff -= line_incr * ( target->rows - 1 );

    line_buff += ( x_offset >> 3 ) + y_offset * line_incr;

    /***********************************************************************/
    /*                                                                     */
    /* We use the extra-classic `accumulator' trick to extract the bits    */
    /* from the source byte stream.                                        */
    /*                                                                     */
    /* Namely, the variable `acc' is a 16-bit accumulator containing the   */
    /* last `loaded' bits from the input stream.  The bits are shifted to  */
    /* the upmost position in `acc'.                                       */
    /*                                                                     */
    /***********************************************************************/

    acc    = 0;  /* clear accumulator   */
    loaded = 0;  /* no bits were loaded */

    for ( height = target->rows; height > 0; height-- )
    {
      FT_Byte*  cur   = line_buff;        /* current write cursor          */
      FT_Int    count = line_bits;        /* # of bits to extract per line */
      FT_Byte   shift = (FT_Byte)( x_offset & 7 ); /* current write shift  */
      FT_Byte   space = (FT_Byte)( 8 - shift );


      /* first of all, read individual source bytes */
      if ( count >= 8 )
      {
        count -= 8;
        {
          do
          {
            FT_Byte  val;


            /* ensure that there are at least 8 bits in the accumulator */
            if ( loaded < 8 )
            {
              acc    |= (FT_UShort)((FT_UShort)*source++ << ( 8 - loaded ));
              loaded += 8;
            }

            /* now write one byte */
            val = (FT_Byte)( acc >> 8 );
            if ( shift )
            {
              cur[0] |= (FT_Byte)(val >> shift);
              cur[1] |= (FT_Byte)(val << space);
            }
            else
              cur[0] |= val;

            cur++;
            acc   <<= 8;  /* remove bits from accumulator */
            loaded -= 8;
            count  -= 8;

          } while ( count >= 0 );
        }

        /* restore `count' to correct value */
        count += 8;
      }

      /* now write remaining bits (count < 8) */
      if ( count > 0 )
      {
        FT_Byte  val;


        /* ensure that there are at least `count' bits in the accumulator */
        if ( (FT_Int)loaded < count )
        {
          acc    |= (FT_UShort)((FT_UShort)*source++ << ( 8 - loaded ));
          loaded += 8;
        }

        /* now write remaining bits */
        val     = (FT_Byte)( ( (FT_Byte)( acc >> 8 ) ) & ~( 0xFF >> count ) );
        cur[0] |= (FT_Byte)(val >> shift);

        if ( count > space )
          cur[1] |= (FT_Byte)(val << space);

        acc   <<= count;
        loaded -= count;
      }

      /* now, skip to next line */
      if ( byte_padded )
      {
        acc    = 0;
        loaded = 0;   /* clear accumulator on byte-padded lines */
      }

      line_buff += line_incr;
    }
  }


  const FT_Frame_Field  sbit_metrics_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_Metrics

    FT_FRAME_START( 8 ),
      FT_FRAME_BYTE( height ),
      FT_FRAME_BYTE( width ),

      FT_FRAME_CHAR( horiBearingX ),
      FT_FRAME_CHAR( horiBearingY ),
      FT_FRAME_BYTE( horiAdvance ),

      FT_FRAME_CHAR( vertBearingX ),
      FT_FRAME_CHAR( vertBearingY ),
      FT_FRAME_BYTE( vertAdvance ),
    FT_FRAME_END
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Const_Metrics                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the metrics for `EBLC' index tables format 2 and 5.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The target range.                                        */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Const_Metrics( TT_SBit_Range*  range,
                           FT_Stream       stream )
  {
    FT_Error  error;


    if ( READ_ULong( range->image_size ) )
      return error;

    return READ_Fields( sbit_metrics_fields, &range->metrics );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Range_Codes                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the range codes for `EBLC' index tables format 4 and 5.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range        :: The target range.                                  */
  /*                                                                       */
  /*    stream       :: The input stream.                                  */
  /*                                                                       */
  /*    load_offsets :: A flag whether to load the glyph offset table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Range_Codes( TT_SBit_Range*  range,
                         FT_Stream       stream,
                         FT_Bool         load_offsets )
  {
    FT_Error   error;
    FT_ULong   count, n, size;
    FT_Memory  memory = stream->memory;


    if ( READ_ULong( count ) )
      goto Exit;

    range->num_glyphs = count;

    /* Allocate glyph offsets table if needed */
    if ( load_offsets )
    {
      if ( ALLOC_ARRAY( range->glyph_offsets, count, FT_ULong ) )
        goto Exit;

      size = count * 4L;
    }
    else
      size = count * 2L;

    /* Allocate glyph codes table and access frame */
    if ( ALLOC_ARRAY ( range->glyph_codes, count, FT_UShort ) ||
         ACCESS_Frame( size )                                 )
      goto Exit;

    for ( n = 0; n < count; n++ )
    {
      range->glyph_codes[n] = GET_UShort();

      if ( load_offsets )
        range->glyph_offsets[n] = (FT_ULong)range->image_offset +
                                  GET_UShort();
    }

    FORGET_Frame();

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Range                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given `EBLC' index/range table.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    range  :: The target range.                                        */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_SBit_Range( TT_SBit_Range*  range,
                   FT_Stream       stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;


    switch( range->index_format )
    {
    case 1:   /* variable metrics with 4-byte offsets */
    case 3:   /* variable metrics with 2-byte offsets */
      {
        FT_ULong  num_glyphs, n;
        FT_Int    size_elem;
        FT_Bool   large = FT_BOOL( range->index_format == 1 );


        num_glyphs        = range->last_glyph - range->first_glyph + 1L;
        range->num_glyphs = num_glyphs;
        num_glyphs++;                       /* XXX: BEWARE - see spec */

        size_elem = large ? 4 : 2;

        if ( ALLOC_ARRAY( range->glyph_offsets,
                          num_glyphs, FT_ULong )    ||
             ACCESS_Frame( num_glyphs * size_elem ) )
          goto Exit;

        for ( n = 0; n < num_glyphs; n++ )
          range->glyph_offsets[n] = (FT_ULong)( range->image_offset +
                                                  ( large ? GET_ULong()
                                                          : GET_UShort() ) );
        FORGET_Frame();
      }
      break;

    case 2:   /* all glyphs have identical metrics */
      error = Load_SBit_Const_Metrics( range, stream );
      break;

    case 4:
      error = Load_SBit_Range_Codes( range, stream, 1 );
      break;

    case 5:
      error = Load_SBit_Const_Metrics( range, stream ) ||
              Load_SBit_Range_Codes( range, stream, 0  );
      break;

    default:
      error = SFNT_Err_Invalid_File_Format;
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Strikes                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table of embedded bitmap sizes for this face.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The target face object.                                  */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_SBit_Strikes( TT_Face    face,
                        FT_Stream  stream )
  {
    FT_Error   error  = 0;
    FT_Memory  memory = stream->memory;
    FT_Fixed   version;
    FT_ULong   num_strikes;
    FT_ULong   table_base;

    const FT_Frame_Field  sbit_line_metrics_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_Line_Metrics

      /* no FT_FRAME_START */
        FT_FRAME_CHAR( ascender ),
        FT_FRAME_CHAR( descender ),
        FT_FRAME_BYTE( max_width ),

        FT_FRAME_CHAR( caret_slope_numerator ),
        FT_FRAME_CHAR( caret_slope_denominator ),
        FT_FRAME_CHAR( caret_offset ),

        FT_FRAME_CHAR( min_origin_SB ),
        FT_FRAME_CHAR( min_advance_SB ),
        FT_FRAME_CHAR( max_before_BL ),
        FT_FRAME_CHAR( min_after_BL ),
        FT_FRAME_CHAR( pads[0] ),
        FT_FRAME_CHAR( pads[1] ),
      FT_FRAME_END
    };

    const FT_Frame_Field  strike_start_fields[] =
    {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_Strike

      /* no FT_FRAME_START */
        FT_FRAME_ULONG( ranges_offset ),
        FT_FRAME_SKIP_LONG,
        FT_FRAME_ULONG( num_ranges ),
        FT_FRAME_ULONG( color_ref ),
      FT_FRAME_END
    };

    const FT_Frame_Field  strike_end_fields[] =
    {
      /* no FT_FRAME_START */
        FT_FRAME_USHORT( start_glyph ),
        FT_FRAME_USHORT( end_glyph ),
        FT_FRAME_BYTE  ( x_ppem ),
        FT_FRAME_BYTE  ( y_ppem ),
        FT_FRAME_BYTE  ( bit_depth ),
        FT_FRAME_CHAR  ( flags ),
      FT_FRAME_END
    };


    face->num_sbit_strikes = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_EBLC, stream, 0 );
    if ( error )
      error = face->goto_table( face, TTAG_bloc, stream, 0 );
    if ( error )
      goto Exit;

    table_base = FILE_Pos();
    if ( ACCESS_Frame( 8L ) )
      goto Exit;

    version     = GET_Long();
    num_strikes = GET_ULong();

    FORGET_Frame();

    /* check version number and strike count */
    if ( version     != 0x00020000L ||
         num_strikes >= 0x10000L    )
    {
      FT_ERROR(( "TT_Load_SBit_Strikes: invalid table version!\n" ));
      error = SFNT_Err_Invalid_File_Format;

      goto Exit;
    }

    /* allocate the strikes table */
    if ( ALLOC_ARRAY( face->sbit_strikes, num_strikes, TT_SBit_Strike ) )
      goto Exit;

    face->num_sbit_strikes = num_strikes;

    /* now read each strike table separately */
    {
      TT_SBit_Strike*  strike = face->sbit_strikes;
      FT_ULong         count  = num_strikes;


      if ( ACCESS_Frame( 48L * num_strikes ) )
        goto Exit;

      while ( count > 0 )
      {
        if ( READ_Fields( strike_start_fields, strike )             ||
             READ_Fields( sbit_line_metrics_fields, &strike->hori ) ||
             READ_Fields( sbit_line_metrics_fields, &strike->vert ) ||
             READ_Fields( strike_end_fields, strike )               )
          break;

        count--;
        strike++;
      }

      FORGET_Frame();
    }

    /* allocate the index ranges for each strike table */
    {
      TT_SBit_Strike*  strike = face->sbit_strikes;
      FT_ULong         count  = num_strikes;


      while ( count > 0 )
      {
        TT_SBit_Range*  range;
        FT_ULong        count2 = strike->num_ranges;


        if ( ALLOC_ARRAY( strike->sbit_ranges,
                          strike->num_ranges,
                          TT_SBit_Range ) )
          goto Exit;

        /* read each range */
        if ( FILE_Seek( table_base + strike->ranges_offset ) ||
             ACCESS_Frame( strike->num_ranges * 8L )         )
          goto Exit;

        range = strike->sbit_ranges;
        while ( count2 > 0 )
        {
          range->first_glyph  = GET_UShort();
          range->last_glyph   = GET_UShort();
          range->table_offset = table_base + strike->ranges_offset
                                 + GET_ULong();
          count2--;
          range++;
        }

        FORGET_Frame();

        /* Now, read each index table */
        count2 = strike->num_ranges;
        range  = strike->sbit_ranges;
        while ( count2 > 0 )
        {
          /* Read the header */
          if ( FILE_Seek( range->table_offset ) ||
               ACCESS_Frame( 8L )               )
            goto Exit;

          range->index_format = GET_UShort();
          range->image_format = GET_UShort();
          range->image_offset = GET_ULong();

          FORGET_Frame();

          error = Load_SBit_Range( range, stream );
          if ( error )
            goto Exit;

          count2--;
          range++;
        }

        count--;
        strike++;
      }
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Free_SBit_Strikes                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases the embedded bitmap tables.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: The target face object.                                    */
  /*                                                                       */
  FT_LOCAL_DEF void
  TT_Free_SBit_Strikes( TT_Face  face )
  {
    FT_Memory        memory       = face->root.memory;
    TT_SBit_Strike*  strike       = face->sbit_strikes;
    TT_SBit_Strike*  strike_limit = strike + face->num_sbit_strikes;


    if ( strike )
    {
      for ( ; strike < strike_limit; strike++ )
      {
        TT_SBit_Range*  range       = strike->sbit_ranges;
        TT_SBit_Range*  range_limit = range + strike->num_ranges;


        if ( range )
        {
          for ( ; range < range_limit; range++ )
          {
            /* release the glyph offsets and codes tables */
            /* where appropriate                          */
            FREE( range->glyph_offsets );
            FREE( range->glyph_codes );
          }
        }
        FREE( strike->sbit_ranges );
        strike->num_ranges = 0;
      }
      FREE( face->sbit_strikes );
    }
    face->num_sbit_strikes = 0;
  }


  FT_LOCAL_DEF FT_Error
  TT_Set_SBit_Strike( TT_Face    face,
                      FT_Int     x_ppem,
                      FT_Int     y_ppem,
                      FT_ULong  *astrike_index )
  {
    FT_Int  i;


    if ( x_ppem < 0 || x_ppem > 255 ||
         y_ppem < 1 || y_ppem > 255 )
      return SFNT_Err_Invalid_PPem;

    for ( i = 0; i < face->num_sbit_strikes; i++ )
    {
      if ( ( face->sbit_strikes[i].y_ppem  == y_ppem )  &&
           ( ( x_ppem == 0 ) ||
             ( face->sbit_strikes[i].x_ppem == x_ppem ) ) )
      {
        *astrike_index = i;
        return SFNT_Err_Ok;
      }
    }

    return SFNT_Err_Invalid_PPem;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Find_SBit_Range                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Scans a given strike's ranges and return, for a given glyph        */
  /*    index, the corresponding sbit range, and `EBDT' offset.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph_index   :: The glyph index.                                  */
  /*                                                                       */
  /*    strike        :: The source/current sbit strike.                   */
  /*                                                                       */
  /* <Output>                                                              */
  /*    arange        :: The sbit range containing the glyph index.        */
  /*                                                                       */
  /*    aglyph_offset :: The offset of the glyph data in `EBDT' table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means the glyph index was found.           */
  /*                                                                       */
  static FT_Error
  Find_SBit_Range( FT_UInt          glyph_index,
                   TT_SBit_Strike*  strike,
                   TT_SBit_Range**  arange,
                   FT_ULong*        aglyph_offset )
  {
    TT_SBit_Range  *range, *range_limit;


    /* check whether the glyph index is within this strike's */
    /* glyph range                                           */
    if ( glyph_index < (FT_UInt)strike->start_glyph ||
         glyph_index > (FT_UInt)strike->end_glyph   )
      goto Fail;

    /* scan all ranges in strike */
    range       = strike->sbit_ranges;
    range_limit = range + strike->num_ranges;
    if ( !range )
      goto Fail;

    for ( ; range < range_limit; range++ )
    {
      if ( glyph_index >= (FT_UInt)range->first_glyph &&
           glyph_index <= (FT_UInt)range->last_glyph  )
      {
        FT_UShort  delta = (FT_UShort)( glyph_index - range->first_glyph );


        switch ( range->index_format )
        {
        case 1:
        case 3:
          *aglyph_offset = range->glyph_offsets[delta];
          break;

        case 2:
          *aglyph_offset = range->image_offset +
                           range->image_size * delta;
          break;

        case 4:
        case 5:
          {
            FT_ULong  n;


            for ( n = 0; n < range->num_glyphs; n++ )
            {
              if ( (FT_UInt)range->glyph_codes[n] == glyph_index )
              {
                if ( range->index_format == 4 )
                  *aglyph_offset = range->glyph_offsets[n];
                else
                  *aglyph_offset = range->image_offset +
                                   n * range->image_size;
                goto Found;
              }
            }
          }

          /* fall-through */
          default:
            goto Fail;
        }

    Found:
        /* return successfully! */
        *arange  = range;
        return 0;
      }
    }

  Fail:
    *arange        = 0;
    *aglyph_offset = 0;

    return SFNT_Err_Invalid_Argument;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Find_SBit_Image                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Checks whether an embedded bitmap (an `sbit') exists for a given   */
  /*    glyph, at a given strike.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    glyph_index   :: The glyph index.                                  */
  /*                                                                       */
  /*    strike_index  :: The current strike index.                         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    arange        :: The SBit range containing the glyph index.        */
  /*                                                                       */
  /*    astrike       :: The SBit strike containing the glyph index.       */
  /*                                                                       */
  /*    aglyph_offset :: The offset of the glyph data in `EBDT' table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns                    */
  /*    SFNT_Err_Invalid_Argument if no sbit exists for the requested      */
  /*    glyph.                                                             */
  /*                                                                       */
  static FT_Error
  Find_SBit_Image( TT_Face           face,
                   FT_UInt           glyph_index,
                   FT_ULong          strike_index,
                   TT_SBit_Range*   *arange,
                   TT_SBit_Strike*  *astrike,
                   FT_ULong         *aglyph_offset )
  {
    FT_Error         error;
    TT_SBit_Strike*  strike;


    if ( !face->sbit_strikes                              ||
         ( face->num_sbit_strikes <= (FT_Int)strike_index ) )
      goto Fail;

    strike = &face->sbit_strikes[strike_index];

    error = Find_SBit_Range( glyph_index, strike,
                             arange, aglyph_offset );
    if ( error )
      goto Fail;

    *astrike = strike;

    return SFNT_Err_Ok;

  Fail:
    /* no embedded bitmap for this glyph in face */
    *arange        = 0;
    *astrike       = 0;
    *aglyph_offset = 0;

    return SFNT_Err_Invalid_Argument;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Load_SBit_Metrics                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Gets the big metrics for a given SBit.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream      :: The input stream.                                   */
  /*                                                                       */
  /*    range       :: The SBit range containing the glyph.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    big_metrics :: A big SBit metrics structure for the glyph.         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be positioned at the glyph's offset within  */
  /*    the `EBDT' table before the call.                                  */
  /*                                                                       */
  /*    If the image format uses variable metrics, the stream cursor is    */
  /*    positioned just after the metrics header in the `EBDT' table on    */
  /*    function exit.                                                     */
  /*                                                                       */
  static FT_Error
  Load_SBit_Metrics( FT_Stream         stream,
                     TT_SBit_Range*    range,
                     TT_SBit_Metrics*  metrics )
  {
    FT_Error  error = SFNT_Err_Ok;


    switch ( range->image_format )
    {
    case 1:
    case 2:
    case 8:
      /* variable small metrics */
      {
        TT_SBit_Small_Metrics  smetrics;

        const FT_Frame_Field  sbit_small_metrics_fields[] =
        {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  TT_SBit_Small_Metrics

          FT_FRAME_START( 5 ),
            FT_FRAME_BYTE( height ),
            FT_FRAME_BYTE( width ),
            FT_FRAME_CHAR( bearingX ),
            FT_FRAME_CHAR( bearingY ),
            FT_FRAME_BYTE( advance ),
          FT_FRAME_END
        };


        /* read small metrics */
        if ( READ_Fields( sbit_small_metrics_fields, &smetrics ) )
          goto Exit;

        /* convert it to a big metrics */
        metrics->height       = smetrics.height;
        metrics->width        = smetrics.width;
        metrics->horiBearingX = smetrics.bearingX;
        metrics->horiBearingY = smetrics.bearingY;
        metrics->horiAdvance  = smetrics.advance;

        /* these metrics are made up at a higher level when */
        /* needed.                                          */
        metrics->vertBearingX = 0;
        metrics->vertBearingY = 0;
        metrics->vertAdvance  = 0;
      }
      break;

    case 6:
    case 7:
    case 9:
      /* variable big metrics */
      if ( READ_Fields( sbit_metrics_fields, metrics ) )
        goto Exit;
      break;

    case 5:
    default:  /* constant metrics */
      if ( range->index_format == 2 || range->index_format == 5 )
        *metrics = range->metrics;
      else
        return SFNT_Err_Invalid_File_Format;
   }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Crop_Bitmap                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Crops a bitmap to its tightest bounding box, and adjusts its       */
  /*    metrics.                                                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    map     :: The bitmap.                                             */
  /*                                                                       */
  /*    metrics :: The corresponding metrics structure.                    */
  /*                                                                       */
  static void
  Crop_Bitmap( FT_Bitmap*        map,
               TT_SBit_Metrics*  metrics )
  {
    /***********************************************************************/
    /*                                                                     */
    /* In this situation, some bounding boxes of embedded bitmaps are too  */
    /* large.  We need to crop it to a reasonable size.                    */
    /*                                                                     */
    /*      ---------                                                      */
    /*      |       |                -----                                 */
    /*      |  ***  |                |***|                                 */
    /*      |   *   |                | * |                                 */
    /*      |   *   |    ------>     | * |                                 */
    /*      |   *   |                | * |                                 */
    /*      |   *   |                | * |                                 */
    /*      |  ***  |                |***|                                 */
    /*      ---------                -----                                 */
    /*                                                                     */
    /***********************************************************************/

    FT_Int    rows, count;
    FT_Long   line_len;
    FT_Byte*  line;


    /***********************************************************************/
    /*                                                                     */
    /* first of all, check the top-most lines of the bitmap, and remove    */
    /* them if they're empty.                                              */
    /*                                                                     */
    {
      line     = (FT_Byte*)map->buffer;
      rows     = map->rows;
      line_len = map->pitch;


      for ( count = 0; count < rows; count++ )
      {
        FT_Byte*  cur   = line;
        FT_Byte*  limit = line + line_len;


        for ( ; cur < limit; cur++ )
          if ( cur[0] )
            goto Found_Top;

        /* the current line was empty - skip to next one */
        line  = limit;
      }

    Found_Top:
      /* check that we have at least one filled line */
      if ( count >= rows )
        goto Empty_Bitmap;

      /* now, crop the empty upper lines */
      if ( count > 0 )
      {
        line = (FT_Byte*)map->buffer;

        MEM_Move( line, line + count * line_len,
                  ( rows - count ) * line_len );

        metrics->height       = (FT_Byte)( metrics->height - count );
        metrics->horiBearingY = (FT_Char)( metrics->horiBearingY - count );
        metrics->vertBearingY = (FT_Char)( metrics->vertBearingY - count );

        map->rows -= count;
        rows      -= count;
      }
    }

    /***********************************************************************/
    /*                                                                     */
    /* second, crop the lower lines                                        */
    /*                                                                     */
    {
      line = (FT_Byte*)map->buffer + ( rows - 1 ) * line_len;

      for ( count = 0; count < rows; count++ )
      {
        FT_Byte*  cur   = line;
        FT_Byte*  limit = line + line_len;


        for ( ; cur < limit; cur++ )
          if ( cur[0] )
            goto Found_Bottom;

        /* the current line was empty - skip to previous one */
        line -= line_len;
      }

    Found_Bottom:
      if ( count > 0 )
      {
        metrics->height  = (FT_Byte)( metrics->height - count );
        rows            -= count;
        map->rows       -= count;
      }
    }

    /***********************************************************************/
    /*                                                                     */
    /* third, get rid of the space on the left side of the glyph           */
    /*                                                                     */
    do
    {
      FT_Byte*  limit;


      line  = (FT_Byte*)map->buffer;
      limit = line + rows * line_len;

      for ( ; line < limit; line += line_len )
        if ( line[0] & 0x80 )
          goto Found_Left;

      /* shift the whole glyph one pixel to the left */
      line  = (FT_Byte*)map->buffer;
      limit = line + rows * line_len;

      for ( ; line < limit; line += line_len )
      {
        FT_Int    n, width = map->width;
        FT_Byte   old;
        FT_Byte*  cur = line;


        old = (FT_Byte)(cur[0] << 1);
        for ( n = 8; n < width; n += 8 )
        {
          FT_Byte  val;


          val    = cur[1];
          cur[0] = (FT_Byte)( old | ( val >> 7 ) );
          old    = (FT_Byte)( val << 1 );
          cur++;
        }
        cur[0] = old;
      }

      map->width--;
      metrics->horiBearingX++;
      metrics->vertBearingX++;
      metrics->width--;

    } while ( map->width > 0 );

  Found_Left:

    /***********************************************************************/
    /*                                                                     */
    /* finally, crop the bitmap width to get rid of the space on the right */
    /* side of the glyph.                                                  */
    /*                                                                     */
    do
    {
      FT_Int    right = map->width - 1;
      FT_Byte*  limit;
      FT_Byte   mask;


      line  = (FT_Byte*)map->buffer + ( right >> 3 );
      limit = line + rows * line_len;
      mask  = (FT_Byte)( 0x80 >> ( right & 7 ) );

      for ( ; line < limit; line += line_len )
        if ( line[0] & mask )
          goto Found_Right;

      /* crop the whole glyph to the right */
      map->width--;
      metrics->width--;

    } while ( map->width > 0 );

  Found_Right:
    /* all right, the bitmap was cropped */
    return;

  Empty_Bitmap:
    map->width      = 0;
    map->rows       = 0;
    map->pitch      = 0;
    map->pixel_mode = ft_pixel_mode_mono;
  }


  static FT_Error
  Load_SBit_Single( FT_Bitmap*        map,
                    FT_Int            x_offset,
                    FT_Int            y_offset,
                    FT_Int            pix_bits,
                    FT_UShort         image_format,
                    TT_SBit_Metrics*  metrics,
                    FT_Stream         stream )
  {
    FT_Error  error;


    /* check that the source bitmap fits into the target pixmap */
    if ( x_offset < 0 || x_offset + metrics->width  > map->width ||
         y_offset < 0 || y_offset + metrics->height > map->rows  )
    {
      error = SFNT_Err_Invalid_Argument;

      goto Exit;
    }

    {
      FT_Int  glyph_width  = metrics->width;
      FT_Int  glyph_height = metrics->height;
      FT_Int  glyph_size;
      FT_Int  line_bits    = pix_bits * glyph_width;
      FT_Bool pad_bytes    = 0;


      /* compute size of glyph image */
      switch ( image_format )
      {
      case 1:  /* byte-padded formats */
      case 6:
        {
          FT_Int  line_length;


          switch ( pix_bits )
          {
          case 1:  line_length = ( glyph_width + 7 ) >> 3;   break;
          case 2:  line_length = ( glyph_width + 3 ) >> 2;   break;
          case 4:  line_length = ( glyph_width + 1 ) >> 1;   break;
          default: line_length =   glyph_width;
          }

          glyph_size = glyph_height * line_length;
          pad_bytes  = 1;
        }
        break;

      case 2:
      case 5:
      case 7:
        line_bits  =   glyph_width  * pix_bits;
        glyph_size = ( glyph_height * line_bits + 7 ) >> 3;
        break;

      default:  /* invalid format */
        return SFNT_Err_Invalid_File_Format;
      }

      /* Now read data and draw glyph into target pixmap       */
      if ( ACCESS_Frame( glyph_size ) )
        goto Exit;

      /* don't forget to multiply `x_offset' by `map->pix_bits' as */
      /* the sbit blitter doesn't make a difference between pixmap */
      /* depths.                                                   */
      blit_sbit( map, (FT_Byte*)stream->cursor, line_bits, pad_bytes,
                 x_offset * pix_bits, y_offset );

      FORGET_Frame();
    }

  Exit:
    return error;
  }


  static FT_Error
  Load_SBit_Image( TT_SBit_Strike*   strike,
                   TT_SBit_Range*    range,
                   FT_ULong          ebdt_pos,
                   FT_ULong          glyph_offset,
                   FT_Bitmap*        map,
                   FT_Int            x_offset,
                   FT_Int            y_offset,
                   FT_Stream         stream,
                   TT_SBit_Metrics*  metrics )
  {
    FT_Memory  memory = stream->memory;
    FT_Error   error;


    /* place stream at beginning of glyph data and read metrics */
    if ( FILE_Seek( ebdt_pos + glyph_offset ) )
      goto Exit;

    error = Load_SBit_Metrics( stream, range, metrics );
    if ( error )
      goto Exit;

    /* this function is recursive.  At the top-level call, the */
    /* field map.buffer is NULL.  We thus begin by finding the */
    /* dimensions of the higher-level glyph to allocate the    */
    /* final pixmap buffer                                     */
    if ( map->buffer == 0 )
    {
      FT_Long  size;


      map->width = metrics->width;
      map->rows  = metrics->height;

      switch ( strike->bit_depth )
      {
      case 1:
        map->pixel_mode = ft_pixel_mode_mono;
        map->pitch      = ( map->width + 7 ) >> 3;
        break;

      case 2:
        map->pixel_mode = ft_pixel_mode_pal2;
        map->pitch      = ( map->width + 3 ) >> 2;
        break;

      case 4:
        map->pixel_mode = ft_pixel_mode_pal4;
        map->pitch      = ( map->width + 1 ) >> 1;
        break;

      case 8:
        map->pixel_mode = ft_pixel_mode_grays;
        map->pitch      = map->width;
        break;

      default:
        return SFNT_Err_Invalid_File_Format;
      }

      size = map->rows * map->pitch;

      /* check that there is no empty image */
      if ( size == 0 )
        goto Exit;     /* exit successfully! */

      if ( ALLOC( map->buffer, size ) )
        goto Exit;
    }

    switch ( range->image_format )
    {
    case 1:  /* single sbit image - load it */
    case 2:
    case 5:
    case 6:
    case 7:
      return Load_SBit_Single( map, x_offset, y_offset, strike->bit_depth,
                               range->image_format, metrics, stream );

    case 8:  /* compound format */
      FT_Skip_Stream( stream, 1L );
      /* fallthrough */

    case 9:
      break;

    default: /* invalid image format */
      return SFNT_Err_Invalid_File_Format;
    }

    /* All right, we have a compound format.  First of all, read */
    /* the array of elements.                                    */
    {
      TT_SBit_Component*  components;
      TT_SBit_Component*  comp;
      FT_UShort           num_components, count;


      if ( READ_UShort( num_components )                                ||
           ALLOC_ARRAY( components, num_components, TT_SBit_Component ) )
        goto Exit;

      count = num_components;

      if ( ACCESS_Frame( 4L * num_components ) )
        goto Fail_Memory;

      for ( comp = components; count > 0; count--, comp++ )
      {
        comp->glyph_code = GET_UShort();
        comp->x_offset   = GET_Char();
        comp->y_offset   = GET_Char();
      }

      FORGET_Frame();

      /* Now recursively load each element glyph */
      count = num_components;
      comp  = components;
      for ( ; count > 0; count--, comp++ )
      {
        TT_SBit_Range*   elem_range;
        TT_SBit_Metrics  elem_metrics;
        FT_ULong         elem_offset;


        /* find the range for this element */
        error = Find_SBit_Range( comp->glyph_code,
                                 strike,
                                 &elem_range,
                                 &elem_offset );
        if ( error )
          goto Fail_Memory;

        /* now load the element, recursively */
        error = Load_SBit_Image( strike,
                                 elem_range,
                                 ebdt_pos,
                                 elem_offset,
                                 map,
                                 x_offset + comp->x_offset,
                                 y_offset + comp->y_offset,
                                 stream,
                                 &elem_metrics );
        if ( error )
          goto Fail_Memory;
      }

    Fail_Memory:
      FREE( components );
    }

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    TT_Load_SBit_Image                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given glyph sbit image from the font resource.  This also  */
  /*    returns its metrics.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face         :: The target face object.                            */
  /*                                                                       */
  /*    strike_index :: The current strike index.                          */
  /*                                                                       */
  /*    glyph_index  :: The current glyph index.                           */
  /*                                                                       */
  /*    load_flags   :: The glyph load flags (the code checks for the flag */
  /*                    FT_LOAD_CROP_BITMAP).                              */
  /*                                                                       */
  /*    stream       :: The input stream.                                  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    map          :: The target pixmap.                                 */
  /*                                                                       */
  /*    metrics      :: A big sbit metrics structure for the glyph image.  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    glyph sbit exists for the index.                                   */
  /*                                                                       */
  /*  <Note>                                                               */
  /*    The `map.buffer' field is always freed before the glyph is loaded. */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  TT_Load_SBit_Image( TT_Face           face,
                      FT_ULong          strike_index,
                      FT_UInt           glyph_index,
                      FT_UInt           load_flags,
                      FT_Stream         stream,
                      FT_Bitmap        *map,
                      TT_SBit_Metrics  *metrics )
  {
    FT_Error         error;
    FT_Memory        memory = stream->memory;
    FT_ULong         ebdt_pos, glyph_offset;

    TT_SBit_Strike*  strike;
    TT_SBit_Range*   range;


    /* Check whether there is a glyph sbit for the current index */
    error = Find_SBit_Image( face, glyph_index, strike_index,
                             &range, &strike, &glyph_offset );
    if ( error )
      goto Exit;

    /* now, find the location of the `EBDT' table in */
    /* the font file                                 */
    error = face->goto_table( face, TTAG_EBDT, stream, 0 );
    if ( error )
      error = face->goto_table( face, TTAG_bdat, stream, 0 );
    if (error)
      goto Exit;

    ebdt_pos = FILE_Pos();

    /* clear the bitmap & load the bitmap */
    if ( face->root.glyph->flags & ft_glyph_own_bitmap )
      FREE( map->buffer );

    map->rows = map->pitch = map->width = 0;

    error = Load_SBit_Image( strike, range, ebdt_pos, glyph_offset,
                             map, 0, 0, stream, metrics );
    if ( error )
      goto Exit;

    /* the glyph slot owns this bitmap buffer */
    face->root.glyph->flags |= ft_glyph_own_bitmap;

    /* setup vertical metrics if needed */
    if ( strike->flags & 1 )
    {
      /* in case of a horizontal strike only */
      FT_Int  advance;


      advance = strike->hori.ascender - strike->hori.descender;

      /* some heuristic values */

      metrics->vertBearingX = (FT_Char)(-metrics->width / 2 );
      metrics->vertBearingY = (FT_Char)( advance / 10 );
      metrics->vertAdvance  = (FT_Char)( advance * 12 / 10 );
    }

    /* Crop the bitmap now, unless specified otherwise */
    if ( load_flags & FT_LOAD_CROP_BITMAP )
      Crop_Bitmap( map, metrics );

  Exit:
    return error;
  }


/* END */
