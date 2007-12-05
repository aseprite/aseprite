/***************************************************************************/
/*                                                                         */
/*  t1driver.c                                                             */
/*                                                                         */
/*    Type 1 driver interface (body).                                      */
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
#include "t1driver.h"
#include "t1gload.h"
#include "t1load.h"

#include "t1errors.h"

#ifndef T1_CONFIG_OPTION_NO_AFM
#include "t1afm.h"
#endif

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H

#include <string.h>     /* for strcmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1driver


  static FT_Error
  t1_get_glyph_name( T1_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max )
  {
    FT_String*  gname;


    gname = face->type1.glyph_names[glyph_index];

    if ( buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)( strlen( gname ) );


      if (len >= buffer_max)
        len = buffer_max - 1;

      MEM_Copy( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_get_name_index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Uses the Type 1 font's `glyph_names' table to find a given glyph   */
  /*    name's glyph index.                                                */
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
  t1_get_name_index( T1_Face     face,
                     FT_String*  glyph_name )
  {
    FT_Int      i;
    FT_String*  gname;


    for ( i = 0; i < face->type1.num_glyphs; i++ )
    {
      gname = face->type1.glyph_names[i];

      if ( !strcmp( glyph_name, gname ) )
        return (FT_UInt)i;
    }

    return 0;
  }


  static const char*
  t1_get_ps_name( T1_Face    face )
  {
    return (const char*) face->type1.font_name;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Interface                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Each driver can provide one or more extensions to the base         */
  /*    FreeType API.  These can be used to access format specific         */
  /*    features (e.g., all TrueType/OpenType resources share a common     */
  /*    file structure and common tables which can be accessed through the */
  /*    `sfnt' interface), or more simply generic ones (e.g., the          */
  /*    `postscript names' interface which can be used to retrieve the     */
  /*     PostScript name of a given glyph index).                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    driver    :: A handle to a driver object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    interface :: A string designing the interface.  Examples are       */
  /*                 `sfnt', `post_names', `charmaps', etc.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A typeless pointer to the extension's interface (normally a table  */
  /*    of function pointers).  Returns NULL if the requested extension    */
  /*    isn't available (i.e., wasn't compiled in the driver at build      */
  /*    time).                                                             */
  /*                                                                       */
  static FT_Module_Interface
  Get_Interface( FT_Driver         driver,
                 const FT_String*  interface )
  {
    FT_UNUSED( driver );
    FT_UNUSED( interface );

    if ( strcmp( (const char*)interface, "glyph_name" ) == 0 )
      return (FT_Module_Interface)t1_get_glyph_name;

    if ( strcmp( (const char*)interface, "name_index" ) == 0 )
      return (FT_Module_Interface)t1_get_name_index;

    if ( strcmp( (const char*)interface, "postscript_name" ) == 0 )
      return (FT_Module_Interface)t1_get_ps_name;

#ifndef T1_CONFIG_OPTION_NO_MM_SUPPORT
    if ( strcmp( (const char*)interface, "get_mm" ) == 0 )
      return (FT_Module_Interface)T1_Get_Multi_Master;

    if ( strcmp( (const char*)interface, "set_mm_design") == 0 )
      return (FT_Module_Interface)T1_Set_MM_Design;

    if ( strcmp( (const char*)interface, "set_mm_blend") == 0 )
      return (FT_Module_Interface)T1_Set_MM_Blend;
#endif
    return 0;
  }


#ifndef T1_CONFIG_OPTION_NO_AFM

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
  /*    kernings are out of scope of this method (the basic driver         */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  static FT_Error
  Get_Kerning( T1_Face     face,
               FT_UInt     left_glyph,
               FT_UInt     right_glyph,
               FT_Vector*  kerning )
  {
    T1_AFM*  afm;


    kerning->x = 0;
    kerning->y = 0;

    afm = (T1_AFM*)face->afm_data;
    if ( afm )
      T1_Get_Kerning( afm, left_glyph, right_glyph, kerning );

    return T1_Err_Ok;
  }


#endif /* T1_CONFIG_OPTION_NO_AFM */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Char_Index                                                     */
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
  Get_Char_Index( FT_CharMap  charmap,
                  FT_Long     charcode )
  {
    T1_Face             face;
    FT_UInt             result = 0;
    PSNames_Interface*  psnames;


    face    = (T1_Face)charmap->face;
    psnames = (PSNames_Interface*)face->psnames;
    if ( psnames )
      switch ( charmap->encoding )
      {
        /*******************************************************************/
        /*                                                                 */
        /* Unicode encoding support                                        */
        /*                                                                 */
      case ft_encoding_unicode:
        /* use the `PSNames' module to synthetize the Unicode charmap */
        result = psnames->lookup_unicode( &face->unicode_map,
                                          (FT_ULong)charcode );

        /* the function returns 0xFFFF if the Unicode charcode has */
        /* no corresponding glyph                                  */
        if ( result == 0xFFFF )
          result = 0;
        goto Exit;

        /*******************************************************************/
        /*                                                                 */
        /* Custom Type 1 encoding                                          */
        /*                                                                 */
      case ft_encoding_adobe_custom:
        {
          T1_Encoding*  encoding = &face->type1.encoding;


          if ( charcode >= encoding->code_first &&
               charcode <= encoding->code_last  )
            result = encoding->char_index[charcode];
          goto Exit;
        }

        /*******************************************************************/
        /*                                                                 */
        /* Adobe Standard & Expert encoding support                        */
        /*                                                                 */
      default:
        if ( charcode < 256 )
        {
          FT_UInt      code;
          FT_Int       n;
          const char*  glyph_name;


          code = psnames->adobe_std_encoding[charcode];
          if ( charmap->encoding == ft_encoding_adobe_expert )
            code = psnames->adobe_expert_encoding[charcode];

          glyph_name = psnames->adobe_std_strings( code );
          if ( !glyph_name )
            break;

          for ( n = 0; n < face->type1.num_glyphs; n++ )
          {
            const char*  gname = face->type1.glyph_names[n];


            if ( gname && gname[0] == glyph_name[0] &&
                 strcmp( gname, glyph_name ) == 0   )
            {
              result = n;
              break;
            }
          }
        }
      }
  Exit:
    return result;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Driver_Class  t1_driver_class =
  {
    {
      ft_module_font_driver      |
      ft_module_driver_scalable  | 
      ft_module_driver_has_hinter,
      
      sizeof( FT_DriverRec ),

      "type1",
      0x10000L,
      0x20000L,

      0,   /* format interface */

      (FT_Module_Constructor)T1_Driver_Init,
      (FT_Module_Destructor) T1_Driver_Done,
      (FT_Module_Requester)  Get_Interface,
    },

    sizeof( T1_FaceRec ),
    sizeof( T1_SizeRec ),
    sizeof( T1_GlyphSlotRec ),

    (FTDriver_initFace)     T1_Face_Init,
    (FTDriver_doneFace)     T1_Face_Done,
    (FTDriver_initSize)     T1_Size_Init,
    (FTDriver_doneSize)     T1_Size_Done,
    (FTDriver_initGlyphSlot)T1_GlyphSlot_Init,
    (FTDriver_doneGlyphSlot)T1_GlyphSlot_Done,

    (FTDriver_setCharSizes) T1_Size_Reset,
    (FTDriver_setPixelSizes)T1_Size_Reset,
    (FTDriver_loadGlyph)    T1_Load_Glyph,
    (FTDriver_getCharIndex) Get_Char_Index,

#ifdef T1_CONFIG_OPTION_NO_AFM
    (FTDriver_getKerning)   0,
    (FTDriver_attachFile)   0,
#else
    (FTDriver_getKerning)   Get_Kerning,
    (FTDriver_attachFile)   T1_Read_AFM,
#endif
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
    return &t1_driver_class;
  }


#endif /* FT_CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
