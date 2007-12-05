/***************************************************************************/
/*                                                                         */
/*  ttdriver.c                                                             */
/*                                                                         */
/*    TrueType font driver implementation (body).                          */
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
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H

#include "ttdriver.h"
#include "ttgload.h"

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttdriver


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
      return TT_Err_Invalid_Face_Handle;

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
    return TT_Err_Ok;

  Found:
    kerning->x = pair->value;
    goto Exit;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           S I Z E S                             ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Set_Char_Sizes                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to reset a size's character sizes (horizontal */
  /*    and vertical) expressed in fractional points.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_width      :: The character width expressed in 26.6           */
  /*                       fractional points.                              */
  /*                                                                       */
  /*    char_height     :: The character height expressed in 26.6          */
  /*                       fractional points.                              */
  /*                                                                       */
  /*    horz_resolution :: The horizontal resolution of the output device. */
  /*                                                                       */
  /*    vert_resolution :: The vertical resolution of the output device.   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size            :: A handle to the target size object.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Set_Char_Sizes( TT_Size     size,
                  FT_F26Dot6  char_width,
                  FT_F26Dot6  char_height,
                  FT_UInt     horz_resolution,
                  FT_UInt     vert_resolution )
  {
    FT_Size_Metrics*  metrics = &size->root.metrics;
    TT_Face           face    = (TT_Face)size->root.face;
    FT_Long           dim_x, dim_y;


    /* This bit flag, when set, indicates that the pixel size must be */
    /* truncated to an integer.  Nearly all TrueType fonts have this  */
    /* bit set, as hinting won't work really well otherwise.          */
    /*                                                                */
    /* However, for those rare fonts who do not set it, we override   */
    /* the default computations performed by the base layer.  I       */
    /* really don't know whether this is useful, but hey, that's the  */
    /* spec :-)                                                       */
    /*                                                                */
    if ( ( face->header.Flags & 8 ) == 0 )
    {
      /* Compute pixel sizes in 26.6 units */
      dim_x = ( char_width  * horz_resolution + 36 ) / 72;
      dim_y = ( char_height * vert_resolution + 36 ) / 72;

      metrics->x_scale = FT_DivFix( dim_x, face->root.units_per_EM );
      metrics->y_scale = FT_DivFix( dim_y, face->root.units_per_EM );

      metrics->x_ppem  = (FT_UShort)( dim_x >> 6 );
      metrics->y_ppem  = (FT_UShort)( dim_y >> 6 );
    }

    size->ttmetrics.valid = FALSE;
#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    size->strike_index    = 0xFFFF;
#endif

    return TT_Reset_Size( size );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Set_Pixel_Sizes                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to reset a size's character sizes (horizontal */
  /*    and vertical) expressed in integer pixels.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pixel_width  :: The character width expressed in integer pixels.   */
  /*                                                                       */
  /*    pixel_height :: The character height expressed in integer pixels.  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size         :: A handle to the target size object.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  Set_Pixel_Sizes( TT_Size  size,
                   FT_UInt  pixel_width,
                   FT_UInt  pixel_height )
  {
    FT_UNUSED( pixel_width );
    FT_UNUSED( pixel_height );

    /* many things have been pre-computed by the base layer */

    size->ttmetrics.valid = FALSE;
#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    size->strike_index    = 0xFFFF;
#endif

    return TT_Reset_Size( size );
  }


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
  Load_Glyph( TT_GlyphSlot  slot,
              TT_Size       size,
              FT_UShort     glyph_index,
              FT_UInt       load_flags )
  {
    FT_Error  error;


    if ( !slot )
      return TT_Err_Invalid_Slot_Handle;

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    /* reset the size object if necessary */
    if ( size )
    {
      /* these two object must have the same parent */
      if ( size->root.face != slot->face )
        return TT_Err_Invalid_Face_Handle;

      if ( !size->ttmetrics.valid )
      {
        if ( FT_SET_ERROR( TT_Reset_Size( size ) ) )
          return error;
      }
    }

    /* now load the glyph outline if necessary */
    error = TT_Load_Glyph( size, slot, glyph_index, load_flags );

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
  Get_Char_Index( TT_CharMap  charmap,
                  FT_Long     charcode )
  {
    FT_Error       error;
    TT_Face        face;
    TT_CMapTable*  cmap;


    cmap = &charmap->cmap;
    face = (TT_Face)charmap->root.face;

    /* Load table if needed */
    if ( !cmap->loaded )
    {
      SFNT_Interface*  sfnt = (SFNT_Interface*)face->sfnt;


      error = sfnt->load_charmap( face, cmap, face->root.stream );
      if ( error )
        return 0;

      cmap->loaded = TRUE;
    }

    if ( cmap->get_index )
      return cmap->get_index( cmap, charcode );
    else
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
  tt_get_interface( TT_Driver    driver,
                    const char*  interface )
  {
    FT_Module        sfntd = FT_Get_Module( driver->root.root.library,
                                            "sfnt" );
    SFNT_Interface*  sfnt;


    /* only return the default interface from the SFNT module */
    if ( sfntd )
    {
      sfnt = (SFNT_Interface*)( sfntd->clazz->module_interface );
      if ( sfnt )
        return sfnt->get_interface( FT_MODULE( driver ), interface );
    }

    return 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_Class  tt_driver_class =
  {
    {
      ft_module_font_driver     |
      ft_module_driver_scalable |
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
      ft_module_driver_has_hinter,
#else
      0,
#endif

      sizeof ( TT_DriverRec ),

      "truetype",      /* driver name                           */
      0x10000L,        /* driver version == 1.0                 */
      0x20000L,        /* driver requires FreeType 2.0 or above */

      (void*)0,        /* driver specific interface */

      (FT_Module_Constructor)TT_Init_Driver,
      (FT_Module_Destructor) TT_Done_Driver,
      (FT_Module_Requester)  tt_get_interface,
    },

    sizeof ( TT_FaceRec ),
    sizeof ( TT_SizeRec ),
    sizeof ( FT_GlyphSlotRec ),


    (FTDriver_initFace)     TT_Init_Face,
    (FTDriver_doneFace)     TT_Done_Face,
    (FTDriver_initSize)     TT_Init_Size,
    (FTDriver_doneSize)     TT_Done_Size,
    (FTDriver_initGlyphSlot)0,
    (FTDriver_doneGlyphSlot)0,

    (FTDriver_setCharSizes) Set_Char_Sizes,
    (FTDriver_setPixelSizes)Set_Pixel_Sizes,
    (FTDriver_loadGlyph)    Load_Glyph,
    (FTDriver_getCharIndex) Get_Char_Index,

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
    return &tt_driver_class;
  }


#endif /* CONFIG_OPTION_DYNAMIC_DRIVERS */


/* END */
