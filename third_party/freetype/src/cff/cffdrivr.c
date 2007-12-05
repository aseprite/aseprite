/***************************************************************************/
/*                                                                         */
/*  cffdrivr.c                                                             */
/*                                                                         */
/*    OpenType font driver implementation (body).                          */
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
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H

#include "cffdrivr.h"
#include "cffgload.h"
#include "cffload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdriver


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                          F A C E S                              ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Kerning                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to return the kerning vector between two      */
  /*    glyphs of the same face.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the source face object.                 */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    kerning     :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this function.  Other layouts, or more sophisticated  */
  /*    kernings, are out of scope of this method (the basic driver        */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  static FT_Error
  Get_Kerning( TT_Face     face,
               FT_UInt     left_glyph,
               FT_UInt     right_glyph,
               FT_Vector*  kerning )
  {
    TT_Kern_0_Pair*  pair;


    if ( !face )
      return CFF_Err_Invalid_Face_Handle;

    kerning->x = 0;
    kerning->y = 0;

    if ( face->kern_pairs )
    {
      /* there are some kerning pairs in this font file! */
      FT_ULong  search_tag = PAIR_TAG( left_glyph, right_glyph );
      FT_Long   left, right;


      left  = 0;
      right = face->num_kern_pairs - 1;

      while ( left <= right )
      {
        FT_Int    middle = left + ( ( right - left ) >> 1 );
        FT_ULong  cur_pair;


        pair     = face->kern_pairs + middle;
        cur_pair = PAIR_TAG( pair->left, pair->right );

        if ( cur_pair == search_tag )
          goto Found;

        if ( cur_pair < search_tag )
          left = middle + 1;
        else
          right = middle - 1;
      }
    }

  Exit:
    return CFF_Err_Ok;

  Found:
    kerning->x = pair->value;
    goto Exit;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Load_Glyph                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to load a glyph within a given glyph slot.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the target slot object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled, loaded, etc.                        */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FTLOAD_??? constants can be used to control the     */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Load_Glyph( CFF_GlyphSlot  slot,
              CFF_Size       size,
              FT_UShort      glyph_index,
              FT_UInt        load_flags )
  {
    FT_Error  error;


    if ( !slot )
      return CFF_Err_Invalid_Slot_Handle;

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    /* reset the size object if necessary */
    if ( size )
    {
      /* these two object must have the same parent */
      if ( size->face != slot->root.face )
        return CFF_Err_Invalid_Face_Handle;
    }

    /* now load the glyph outline if necessary */
    error = CFF_Load_Glyph( slot, size, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****             C H A R A C T E R   M A P P I N G S                 ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Error
  cff_get_glyph_name( CFF_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    CFF_Font*           font   = (CFF_Font*)face->extra.data;
    FT_Memory           memory = FT_FACE_MEMORY(face);
    FT_String*          gname;
    FT_UShort           sid;
    PSNames_Interface*  psnames;
    FT_Error            error;

    psnames = (PSNames_Interface*)FT_Get_Module_Interface(
                face->root.driver->root.library, "psnames" );

    if ( !psnames )
    {
      FT_ERROR(( "CFF_Init_Face:" ));
      FT_ERROR(( " cannot open CFF & CEF fonts\n" ));
      FT_ERROR(( "             " ));
      FT_ERROR(( " without the `PSNames' module\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    /* first, locate the sid in the charset table */
    sid = font->charset.sids[glyph_index];

    /* now, lookup the name itself */
    gname = CFF_Get_String( &font->string_index, sid, psnames );

    if ( buffer_max > 0 )
    {
      FT_UInt  len = strlen( gname );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      MEM_Copy( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    FREE ( gname );
    error = CFF_Err_Ok;

    Exit:
      return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_get_char_index                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses a charmap to return a given character code's glyph index.     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap  :: A handle to the source charmap object.                 */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index.  0 means `undefined character code'.                  */
  /*                                                                       */
  static FT_UInt
  cff_get_char_index( TT_CharMap  charmap,
                      FT_Long     charcode )
  {
    FT_Error       error;
    CFF_Face       face;
    TT_CMapTable*  cmap;


    cmap = &charmap->cmap;
    face = (CFF_Face)charmap->root.face;

    /* Load table if needed */
    if ( !cmap->loaded )
    {
      SFNT_Interface*  sfnt = (SFNT_Interface*)face->sfnt;


      error = sfnt->load_charmap( face, cmap, face->root.stream );
      if ( error )
        return 0;

      cmap->loaded = TRUE;
    }

    return ( cmap->get_index ? cmap->get_index( cmap, charcode ) : 0 );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_get_name_index                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses the psnames module and the CFF font's charset to to return a  */
  /*    a given glyph name's glyph index.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face object.                  */
  /*                                                                       */
  /*    glyph_name :: The glyph name.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Glyph index.  0 means `undefined character code'.                  */
  /*                                                                       */
  static FT_UInt
  cff_get_name_index( CFF_Face    face,
                      FT_String*  glyph_name )
  {
    CFF_Font*           cff;
    CFF_Charset*        charset;
    PSNames_Interface*  psnames;
    FT_String*          name;
    FT_UShort           sid;
    FT_UInt             i;


    cff     = face->extra.data;
    charset = &cff->charset;

    psnames = (PSNames_Interface*)FT_Get_Module_Interface(
                face->root.driver->root.library, "psnames" );

    for ( i = 0; i < cff->num_glyphs; i++ )
    {
      sid = charset->sids[i];

      if ( sid > 390 )
        name = CFF_Get_Name( &cff->string_index, sid - 391 );
      else
        name = (FT_String *)psnames->adobe_std_strings( sid );

      if ( !strcmp( glyph_name, name ) )
        return i;
    }

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                D R I V E R  I N T E R F A C E                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static FT_Module_Interface
  cff_get_interface( CFF_Driver   driver,
                     const char*  interface )
  {
    FT_Module  sfnt;

#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES

    if ( strcmp( (const char*)interface, "glyph_name" ) == 0 )
      return (FT_Module_Interface)cff_get_glyph_name;

    if ( strcmp( (const char*)interface, "name_index" ) == 0 )
      return (FT_Module_Interface)cff_get_name_index;

#endif

    /* we simply pass our request to the `sfnt' module */
    sfnt = FT_Get_Module( driver->root.root.library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, interface ) : 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_Class  cff_driver_class =
  {
    /* begin with the FT_Module_Class fields */
    {
      ft_module_font_driver       |
      ft_module_driver_scalable   |
      ft_module_driver_has_hinter,
      
      sizeof( CFF_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      0,   /* module-specific interface */

      (FT_Module_Constructor)CFF_Driver_Init,
      (FT_Module_Destructor) CFF_Driver_Done,
      (FT_Module_Requester)  cff_get_interface,
    },

    /* now the specific driver fields */
    sizeof( TT_FaceRec ),
    sizeof( FT_SizeRec ),
    sizeof( CFF_GlyphSlotRec ),

    (FTDriver_initFace)     CFF_Face_Init,
    (FTDriver_doneFace)     CFF_Face_Done,
    (FTDriver_initSize)     CFF_Size_Init,
    (FTDriver_doneSize)     CFF_Size_Done,
    (FTDriver_initGlyphSlot)CFF_GlyphSlot_Init,
    (FTDriver_doneGlyphSlot)CFF_GlyphSlot_Done,

    (FTDriver_setCharSizes) CFF_Size_Reset,
    (FTDriver_setPixelSizes)CFF_Size_Reset,

    (FTDriver_loadGlyph)    Load_Glyph,
    (FTDriver_getCharIndex) cff_get_char_index,

    (FTDriver_getKerning)   Get_Kerning,
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
    return &cff_driver_class;
  }


#endif /* CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
