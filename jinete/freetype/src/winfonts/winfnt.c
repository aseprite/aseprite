/***************************************************************************/
/*                                                                         */
/*  winfnt.c                                                               */
/*                                                                         */
/*    FreeType font driver for Windows FNT/FON files                       */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_FNT_TYPES_H

#include "winfnt.h"

#include "fnterrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_winfnt


  static
  const FT_Frame_Field  winmz_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WinMZ_Header

    FT_FRAME_START( 64 ),
      FT_FRAME_USHORT_LE ( magic ),
      FT_FRAME_SKIP_BYTES( 29 * 2 ),
      FT_FRAME_ULONG_LE  ( lfanew ),
    FT_FRAME_END
  };

  static
  const FT_Frame_Field  winne_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WinNE_Header

    FT_FRAME_START( 40 ),
      FT_FRAME_USHORT_LE ( magic ),
      FT_FRAME_SKIP_BYTES( 34 ),
      FT_FRAME_USHORT_LE ( resource_tab_offset ),
      FT_FRAME_USHORT_LE ( rname_tab_offset ),
    FT_FRAME_END
  };

  static
  const FT_Frame_Field  winfnt_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WinFNT_Header

    FT_FRAME_START( 134 ),
      FT_FRAME_USHORT_LE( version ),
      FT_FRAME_ULONG_LE ( file_size ),
      FT_FRAME_BYTES    ( copyright, 60 ),
      FT_FRAME_USHORT_LE( file_type ),
      FT_FRAME_USHORT_LE( nominal_point_size ),
      FT_FRAME_USHORT_LE( vertical_resolution ),
      FT_FRAME_USHORT_LE( horizontal_resolution ),
      FT_FRAME_USHORT_LE( ascent ),
      FT_FRAME_USHORT_LE( internal_leading ),
      FT_FRAME_USHORT_LE( external_leading ),
      FT_FRAME_BYTE     ( italic ),
      FT_FRAME_BYTE     ( underline ),
      FT_FRAME_BYTE     ( strike_out ),
      FT_FRAME_USHORT_LE( weight ),
      FT_FRAME_BYTE     ( charset ),
      FT_FRAME_USHORT_LE( pixel_width ),
      FT_FRAME_USHORT_LE( pixel_height ),
      FT_FRAME_BYTE     ( pitch_and_family ),
      FT_FRAME_USHORT_LE( avg_width ),
      FT_FRAME_USHORT_LE( max_width ),
      FT_FRAME_BYTE     ( first_char ),
      FT_FRAME_BYTE     ( last_char ),
      FT_FRAME_BYTE     ( default_char ),
      FT_FRAME_BYTE     ( break_char ),
      FT_FRAME_USHORT_LE( bytes_per_row ),
      FT_FRAME_ULONG_LE ( device_offset ),
      FT_FRAME_ULONG_LE ( face_name_offset ),
      FT_FRAME_ULONG_LE ( bits_pointer ),
      FT_FRAME_ULONG_LE ( bits_offset ),
      FT_FRAME_BYTE     ( reserved ),
      FT_FRAME_ULONG_LE ( flags ),
      FT_FRAME_USHORT_LE( A_space ),
      FT_FRAME_USHORT_LE( B_space ),
      FT_FRAME_USHORT_LE( C_space ),
      FT_FRAME_USHORT_LE( color_table_offset ),
      FT_FRAME_BYTES    ( reserved, 4 ),
    FT_FRAME_END
  };


  static void
  fnt_font_done( FNT_Font*  font,
                 FT_Stream  stream )
  {
    if ( font->fnt_frame )
      RELEASE_Frame( font->fnt_frame );

    font->fnt_size  = 0;
    font->fnt_frame = 0;
  }


  static FT_Error
  fnt_font_load( FNT_Font*  font,
                 FT_Stream  stream )
  {
    FT_Error        error;
    WinFNT_Header*  header = &font->header;


    /* first of all, read the FNT header */
    if ( FILE_Seek( font->offset )                   ||
         READ_Fields( winfnt_header_fields, header ) )
      goto Exit;

    /* check header */
    if ( header->version != 0x200 &&
         header->version != 0x300 )
    {
      FT_TRACE2(( "[not a valid FNT file]\n" ));
      error = FNT_Err_Unknown_File_Format;
      goto Exit;
    }

    if ( header->file_type & 1 )
    {
      FT_TRACE2(( "[can't handle vector FNT fonts]\n" ));
      error = FNT_Err_Unknown_File_Format;
      goto Exit;
    }

    /* small fixup -- some fonts have the `pixel_width' field set to 0 */
    if ( header->pixel_width == 0 )
      header->pixel_width = header->pixel_height;

    /* this is a FNT file/table, we now extract its frame */
    if ( FILE_Seek( font->offset )                           ||
         EXTRACT_Frame( header->file_size, font->fnt_frame ) )
      goto Exit;

  Exit:
    return error;
  }


  static void
  fnt_face_done_fonts( FNT_Face  face )
  {
    FT_Memory  memory = FT_FACE(face)->memory;
    FT_Stream  stream = FT_FACE(face)->stream;
    FNT_Font*  cur    = face->fonts;
    FNT_Font*  limit  = cur + face->num_fonts;


    for ( ; cur < limit; cur++ )
      fnt_font_done( cur, stream );

    FREE( face->fonts );
    face->num_fonts = 0;
  }


  static FT_Error
  fnt_face_get_dll_fonts( FNT_Face  face )
  {
    FT_Error      error;
    FT_Stream     stream = FT_FACE(face)->stream;
    FT_Memory     memory = FT_FACE(face)->memory;
    WinMZ_Header  mz_header;


    face->fonts     = 0;
    face->num_fonts = 0;

    /* does it begin with a MZ header? */
    if ( FILE_Seek( 0 )                                 ||
         READ_Fields( winmz_header_fields, &mz_header ) )
      goto Exit;

    error = FNT_Err_Unknown_File_Format;
    if ( mz_header.magic == WINFNT_MZ_MAGIC )
    {
      /* yes, now look for a NE header in the file */
      WinNE_Header  ne_header;


      if ( FILE_Seek( mz_header.lfanew )                  ||
           READ_Fields( winne_header_fields, &ne_header ) )
        goto Exit;

      error = FNT_Err_Unknown_File_Format;
      if ( ne_header.magic == WINFNT_NE_MAGIC )
      {
        /* good, now look in the resource table for each FNT resource */
        FT_ULong   res_offset = mz_header.lfanew +
                                ne_header.resource_tab_offset;

        FT_UShort  size_shift;
        FT_UShort  font_count  = 0;
        FT_ULong   font_offset = 0;


        if ( FILE_Seek( res_offset ) ||
             ACCESS_Frame( ne_header.rname_tab_offset -
                           ne_header.resource_tab_offset ) )
          goto Exit;

        size_shift = GET_UShortLE();

        for (;;)
        {
          FT_UShort  type_id, count;


          type_id = GET_UShortLE();
          if ( !type_id )
            break;

          count = GET_UShortLE();

          if ( type_id == 0x8008 )
          {
            font_count  = count;
            font_offset = (FT_ULong)( FILE_Pos() + 4 +
                                      ( stream->cursor - stream->limit ) );
            break;
          }

          stream->cursor += 4 + count * 12;
        }
        FORGET_Frame();

        if ( !font_count || !font_offset )
        {
          FT_TRACE2(( "this file doesn't contain any FNT resources!\n" ));
          error = FNT_Err_Unknown_File_Format;
          goto Exit;
        }

        if ( FILE_Seek( font_offset )                         ||
             ALLOC_ARRAY( face->fonts, font_count, FNT_Font ) )
          goto Exit;

        face->num_fonts = font_count;

        if ( ACCESS_Frame( (FT_Long)font_count * 12 ) )
          goto Exit;

        /* now read the offset and position of each FNT font */
        {
          FNT_Font*  cur   = face->fonts;
          FNT_Font*  limit = cur + font_count;


          for ( ; cur < limit; cur++ )
          {
            cur->offset     = (FT_ULong)GET_UShortLE() << size_shift;
            cur->fnt_size   = (FT_ULong)GET_UShortLE() << size_shift;
            cur->size_shift = size_shift;
            stream->cursor += 8;
          }
        }
        FORGET_Frame();

        /* finally, try to load each font there */
        {
          FNT_Font*  cur   = face->fonts;
          FNT_Font*  limit = cur + font_count;


          for ( ; cur < limit; cur++ )
          {
            error = fnt_font_load( cur, stream );
            if ( error )
              goto Fail;
          }
        }
      }
    }

  Fail:
    if ( error )
      fnt_face_done_fonts( face );

  Exit:
    return error;
  }


  static void
  FNT_Face_Done( FNT_Face  face )
  {
    FT_Memory  memory = FT_FACE_MEMORY( face );


    fnt_face_done_fonts( face );

    FREE( face->root.available_sizes );
    face->root.num_fixed_sizes = 0;
  }


  static FT_Error
  FNT_Face_Init( FT_Stream      stream,
                 FNT_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error   error;
    FT_Memory  memory = FT_FACE_MEMORY( face );

    FT_UNUSED( num_params );
    FT_UNUSED( params );
    FT_UNUSED( face_index );


    /* try to load several fonts from a DLL */
    error = fnt_face_get_dll_fonts( face );
    if ( error )
    {
      /* this didn't work, now try to load a single FNT font */
      FNT_Font*  font;

      if ( ALLOC( face->fonts, sizeof ( *face->fonts ) ) )
        goto Exit;

      face->num_fonts = 1;
      font            = face->fonts;

      font->offset   = 0;
      font->fnt_size = stream->size;

      error = fnt_font_load( font, stream );
      if ( error )
        goto Fail;
    }

    /* all right, one or more fonts were loaded; we now need to */
    /* fill the root FT_Face fields with relevant information   */
    {
      FT_Face    root  = FT_FACE( face );
      FNT_Font*  fonts = face->fonts;
      FNT_Font*  limit = fonts + face->num_fonts;
      FNT_Font*  cur;


      root->num_faces  = 1;
      root->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                         FT_FACE_FLAG_HORIZONTAL;

      if ( fonts->header.avg_width == fonts->header.max_width )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      if ( fonts->header.italic )
        root->style_flags |= FT_STYLE_FLAG_ITALIC;

      if ( fonts->header.weight >= 800 )
        root->style_flags |= FT_STYLE_FLAG_BOLD;

      /* Setup the `fixed_sizes' array */
      if ( ALLOC_ARRAY( root->available_sizes, face->num_fonts,
                        FT_Bitmap_Size ) )
        goto Fail;

      root->num_fixed_sizes = face->num_fonts;

      {
        FT_Bitmap_Size*  size = root->available_sizes;


        for ( cur = fonts; cur < limit; cur++, size++ )
        {
          size->width  = cur->header.pixel_width;
          size->height = cur->header.pixel_height;
        }
      }

      /* Setup the `charmaps' array */
      root->charmaps     = &face->charmap_handle;
      root->num_charmaps = 1;

      face->charmap.encoding    = ft_encoding_unicode;
      face->charmap.platform_id = 3;
      face->charmap.encoding_id = 1;
      face->charmap.face        = root;

      face->charmap_handle = &face->charmap;

      root->charmap = face->charmap_handle;

      /* setup remaining flags */
      root->num_glyphs = fonts->header.last_char -
                         fonts->header.first_char + 1;

      root->family_name = (FT_String*)fonts->fnt_frame +
                          fonts->header.face_name_offset;
      root->style_name  = (char *)"Regular";

      if ( root->style_flags & FT_STYLE_FLAG_BOLD )
      {
        if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
          root->style_name = (char *)"Bold Italic";
        else
          root->style_name = (char *)"Bold";
      }
      else if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
        root->style_name = (char *)"Italic";
    }

  Fail:
    if ( error )
      FNT_Face_Done( face );

  Exit:
    return error;
  }


  static FT_Error
  FNT_Size_Set_Pixels( FNT_Size  size )
  {
    /* look up a font corresponding to the current pixel size */
    FNT_Face   face  = (FNT_Face)FT_SIZE_FACE( size );
    FNT_Font*  cur   = face->fonts;
    FNT_Font*  limit = cur + face->num_fonts;


    size->font = 0;
    for ( ; cur < limit; cur++ )
    {
      /* we only compare the character height, as fonts used some strange */
      /* values                                                           */
      if ( cur->header.pixel_height == size->root.metrics.y_ppem )
      {
        size->font = cur;

        size->root.metrics.ascender  = cur->header.ascent * 64;
        size->root.metrics.descender = ( cur->header.pixel_height -
                                           cur->header.ascent ) * 64;
        size->root.metrics.height    = cur->header.pixel_height * 64;
        break;
      }
    }

    return ( size->font ? FNT_Err_Ok : FNT_Err_Invalid_Pixel_Size );
  }


  static FT_UInt
  FNT_Get_Char_Index( FT_CharMap  charmap,
                      FT_Long     char_code )
  {
    FT_Long  result = char_code;


    if ( charmap )
    {
      FNT_Font*  font  = ((FNT_Face)charmap->face)->fonts;
      FT_Long    first = font->header.first_char;
      FT_Long    count = font->header.last_char - first + 1;


      char_code -= first;
      if ( char_code < count )
        result = char_code + 1;
      else
        result = 0;
    }

    return result;
  }


  static FT_Error
  FNT_Load_Glyph( FT_GlyphSlot  slot,
                  FNT_Size      size,
                  FT_UInt       glyph_index,
                  FT_Int        load_flags )
  {
    FNT_Font*   font  = size->font;
    FT_Error    error = 0;
    FT_Byte*    p;
    FT_Int      len;
    FT_Bitmap*  bitmap = &slot->bitmap;
    FT_ULong    offset;
    FT_Bool     new_format;

    FT_UNUSED( slot );
    FT_UNUSED( load_flags );


    if ( !font )
    {
      error = FNT_Err_Invalid_Argument;
      goto Exit;
    }

    if ( glyph_index > 0 )
      glyph_index--;
    else
      glyph_index = font->header.default_char - font->header.first_char;

    new_format = FT_BOOL( font->header.version == 0x300 );
    len        = new_format ? 6 : 4;

    /* jump to glyph entry */
    p = font->fnt_frame + 118 + len * glyph_index;

    bitmap->width = NEXT_ShortLE(p);

    if ( new_format )
      offset = NEXT_ULongLE(p);
    else
      offset = NEXT_UShortLE(p);

    /* jump to glyph data */
    p = font->fnt_frame + /* font->header.bits_offset */ + offset;

    /* allocate and build bitmap */
    {
      FT_Memory  memory = FT_FACE_MEMORY( slot->face );
      FT_Int     pitch  = ( bitmap->width + 7 ) >> 3;
      FT_Byte*   column;
      FT_Byte*   write;


      bitmap->pitch      = pitch;
      bitmap->rows       = font->header.pixel_height;
      bitmap->pixel_mode = ft_pixel_mode_mono;

      if ( ALLOC( bitmap->buffer, pitch * bitmap->rows ) )
        goto Exit;

      column = (FT_Byte*)bitmap->buffer;

      for ( ; pitch > 0; pitch--, column++ )
      {
        FT_Byte*  limit = p + bitmap->rows;


        for ( write = column; p < limit; p++, write += bitmap->pitch )
          write[0] = p[0];
      }
    }

    slot->flags       = ft_glyph_own_bitmap;
    slot->bitmap_left = 0;
    slot->bitmap_top  = font->header.ascent;
    slot->format      = ft_glyph_format_bitmap;

    /* now set up metrics */
    slot->metrics.horiAdvance  = bitmap->width << 6;
    slot->metrics.horiBearingX = 0;
    slot->metrics.horiBearingY = slot->bitmap_top << 6;

    slot->linearHoriAdvance    = (FT_Fixed)bitmap->width << 16;
    slot->format               = ft_glyph_format_bitmap;

  Exit:
    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_Class  winfnt_driver_class =
  {
    {
      ft_module_font_driver,
      sizeof ( FT_DriverRec ),

      "winfonts",
      0x10000L,
      0x20000L,

      0,

      (FT_Module_Constructor)0,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    sizeof( FNT_FaceRec ),
    sizeof( FNT_SizeRec ),
    sizeof( FT_GlyphSlotRec ),

    (FTDriver_initFace)     FNT_Face_Init,
    (FTDriver_doneFace)     FNT_Face_Done,
    (FTDriver_initSize)     0,
    (FTDriver_doneSize)     0,
    (FTDriver_initGlyphSlot)0,
    (FTDriver_doneGlyphSlot)0,

    (FTDriver_setCharSizes) FNT_Size_Set_Pixels,
    (FTDriver_setPixelSizes)FNT_Size_Set_Pixels,

    (FTDriver_loadGlyph)    FNT_Load_Glyph,
    (FTDriver_getCharIndex) FNT_Get_Char_Index,

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
    return &winfnt_driver_class;
  }


#endif /* FT_CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
