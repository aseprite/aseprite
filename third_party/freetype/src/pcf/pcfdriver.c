/*  pcfdriver.c

    FreeType font driver for pcf files

    Copyright (C) 2000-2001 by
    Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <ft2build.h>

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "pcf.h"
#include "pcfdriver.h"
#include "pcfutil.h"

#include "pcferror.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pcfdriver


  FT_LOCAL_DEF FT_Error
  PCF_Done_Face( PCF_Face  face )
  {
    FT_Memory    memory = FT_FACE_MEMORY( face );
    PCF_Property tmp    = face->properties;
    int i;


    FREE( face->encodings );
    FREE( face->metrics );

    for ( i = 0; i < face->nprops; i++ )
    {
      FREE( tmp->name );
      if ( tmp->isString )
        FREE( tmp->value );
    }
    FREE( face->properties );

    FT_TRACE4(( "DONE_FACE!!!\n" ));

    return PCF_Err_Ok;
  }


  static FT_Error
  PCF_Init_Face( FT_Stream      stream,
                 PCF_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error  error = PCF_Err_Ok;

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );


    error = pcf_load_font( stream, face );
    if ( error )
      goto Fail;

    return PCF_Err_Ok;

  Fail:
    FT_TRACE2(( "[not a valid PCF file]\n" ));
    PCF_Done_Face( face );

    return PCF_Err_Unknown_File_Format; /* error */
  }


  static FT_Error
  PCF_Set_Pixel_Size( FT_Size  size )
  {
    PCF_Face face = (PCF_Face)FT_SIZE_FACE( size );


    FT_TRACE4(( "rec %d - pres %d\n", size->metrics.y_ppem,
                                      face->root.available_sizes->height ));

    if ( size->metrics.y_ppem == face->root.available_sizes->height )
    {
      size->metrics.ascender  = face->accel.fontAscent << 6;
      size->metrics.descender = face->accel.fontDescent * (-64);
#if 0
      size->metrics.height    = face->accel.maxbounds.ascent << 6;
#endif
      size->metrics.height    = size->metrics.ascender -
                                size->metrics.descender;

      size->metrics.max_advance = face->accel.maxbounds.characterWidth << 6;

      return PCF_Err_Ok;
    }
    else
    {
      FT_TRACE4(( "size WRONG\n" ));
      return PCF_Err_Invalid_Pixel_Size;
    }
  }


  static FT_Error
  PCF_Load_Glyph( FT_GlyphSlot  slot,
                  FT_Size       size,
                  FT_UInt       glyph_index,
                  FT_Int        load_flags )
  {
    PCF_Face    face   = (PCF_Face)FT_SIZE_FACE( size );
    FT_Error    error  = PCF_Err_Ok;
    FT_Memory   memory = FT_FACE(face)->memory;
    FT_Bitmap*  bitmap = &slot->bitmap;
    PCF_Metric  metric;
    int         bytes;

    FT_Stream   stream = face->root.stream;

    FT_UNUSED( load_flags );


    FT_TRACE4(( "load_glyph %d ---", glyph_index ));

    if ( !face )
    {
      error = PCF_Err_Invalid_Argument;
      goto Exit;
    }

    metric = face->metrics + glyph_index;

    bitmap->rows       = metric->ascent + metric->descent;
    bitmap->width      = metric->rightSideBearing - metric->leftSideBearing;
    bitmap->num_grays  = 1;
    bitmap->pixel_mode = ft_pixel_mode_mono;

    FT_TRACE6(( "BIT_ORDER %d ; BYTE_ORDER %d ; GLYPH_PAD %d\n",
                  PCF_BIT_ORDER( face->bitmapsFormat ),
                  PCF_BYTE_ORDER( face->bitmapsFormat ),
                  PCF_GLYPH_PAD( face->bitmapsFormat ) ));

    switch ( PCF_GLYPH_PAD( face->bitmapsFormat ) )
    {
    case 1:
      bitmap->pitch = ( bitmap->width + 7 ) >> 3;
      break;

    case 2:
      bitmap->pitch = ( ( bitmap->width + 15 ) >> 4 ) << 1;
      break;

    case 4:
      bitmap->pitch = ( ( bitmap->width + 31 ) >> 5 ) << 2;
      break;

    case 8:
      bitmap->pitch = ( ( bitmap->width + 63 ) >> 6 ) << 3;
      break;

    default:
      return PCF_Err_Invalid_File_Format;
    }

    /* XXX: to do: are there cases that need repadding the bitmap? */
    bytes = bitmap->pitch * bitmap->rows;

    if ( ALLOC( bitmap->buffer, bytes ) )
      goto Exit;

    if ( FILE_Seek( metric->bits )        ||
         FILE_Read( bitmap->buffer, bytes ) )
      goto Exit;

    if ( PCF_BIT_ORDER( face->bitmapsFormat ) != MSBFirst )
      BitOrderInvert( bitmap->buffer,bytes );

    if ( ( PCF_BYTE_ORDER( face->bitmapsFormat ) !=
           PCF_BIT_ORDER( face->bitmapsFormat )  ) )
    {
      switch ( PCF_SCAN_UNIT( face->bitmapsFormat ) )
      {
      case 1:
        break;

      case 2:
        TwoByteSwap( bitmap->buffer, bytes );
        break;

      case 4:
        FourByteSwap( bitmap->buffer, bytes );
        break;
      }
    }

    slot->bitmap_left = metric->leftSideBearing;
    slot->bitmap_top  = metric->ascent;

    slot->metrics.horiAdvance  = metric->characterWidth << 6 ;
    slot->metrics.horiBearingX = metric->rightSideBearing << 6 ;
    slot->metrics.horiBearingY = metric->ascent << 6 ;
    slot->metrics.width        = metric->characterWidth << 6 ;
    slot->metrics.height       = bitmap->rows << 6;

    slot->linearHoriAdvance = (FT_Fixed)bitmap->width << 16;
    slot->format            = ft_glyph_format_bitmap;
    slot->flags             = ft_glyph_own_bitmap;

    FT_TRACE4(( " --- ok\n" ));

  Exit:
    return error;
  }


  static FT_UInt
  PCF_Get_Char_Index( FT_CharMap  charmap,
                      FT_Long     char_code )
  {
    PCF_Face      face     = (PCF_Face)charmap->face;
    PCF_Encoding  en_table = face->encodings;
    int           low, high, mid;


    FT_TRACE4(( "get_char_index %ld\n", char_code ));

    low = 0;
    high = face->nencodings - 1;
    while ( low <= high )
    {
      mid = ( low + high ) / 2;
      if ( char_code < en_table[mid].enc )
        high = mid - 1;
      else if ( char_code > en_table[mid].enc )
        low = mid + 1;
      else
        return en_table[mid].glyph;
    }

    return 0;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_Class  pcf_driver_class =
  {
    {
      ft_module_font_driver,
      sizeof ( FT_DriverRec ),

      "pcf",
      0x10000L,
      0x20000L,

      0,

      (FT_Module_Constructor)0,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    sizeof( PCF_FaceRec ),
    sizeof( FT_SizeRec ),
    sizeof( FT_GlyphSlotRec ),

    (FTDriver_initFace)     PCF_Init_Face,
    (FTDriver_doneFace)     PCF_Done_Face,
    (FTDriver_initSize)     0,
    (FTDriver_doneSize)     0,
    (FTDriver_initGlyphSlot)0,
    (FTDriver_doneGlyphSlot)0,

    (FTDriver_setCharSizes) PCF_Set_Pixel_Size,
    (FTDriver_setPixelSizes)PCF_Set_Pixel_Size,

    (FTDriver_loadGlyph)    PCF_Load_Glyph,
    (FTDriver_getCharIndex) PCF_Get_Char_Index,

    (FTDriver_getKerning)   0,
    (FTDriver_attachFile)   0,
    (FTDriver_getAdvances)  0
  };


#ifdef FT_CONFIG_OPTION_DYNAMIC_DRIVERS


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    getDriverClass                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used when compiling the TrueType driver as a      */
  /*    shared library (`.DLL' or `.so').  It will be used by the          */
  /*    high-level library of FreeType to retrieve the address of the      */
  /*    driver's generic interface.                                        */
  /*                                                                       */
  /*    It shouldn't be implemented in a static build, as each driver must */
  /*    have the same function as an exported entry point.                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the TrueType's driver generic interface.  The       */
  /*    format-specific interface can then be retrieved through the method */
  /*    interface->get_format_interface.                                   */
  /*                                                                       */
  FT_EXPORT_DEF( const FT_Driver_Class* )
  getDriverClass( void )
  {
    return &pcf_driver_class;
  }


#endif /* FT_CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
