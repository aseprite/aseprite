/***************************************************************************/
/*                                                                         */
/*  ftdriver.h                                                             */
/*                                                                         */
/*    FreeType font driver interface (specification).                      */
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


#ifndef __FTDRIVER_H__
#define __FTDRIVER_H__


#include <ft2build.h>
#include FT_MODULE_H


FT_BEGIN_HEADER


  typedef FT_Error
  (*FTDriver_initFace)( FT_Stream      stream,
                        FT_Face        face,
                        FT_Int         typeface_index,
                        FT_Int         num_params,
                        FT_Parameter*  parameters );

  typedef void
  (*FTDriver_doneFace)( FT_Face  face );


  typedef FT_Error
  (*FTDriver_initSize)( FT_Size  size );

  typedef void
  (*FTDriver_doneSize)( FT_Size  size );


  typedef FT_Error
  (*FTDriver_initGlyphSlot)( FT_GlyphSlot  slot );

  typedef void
  (*FTDriver_doneGlyphSlot)( FT_GlyphSlot  slot );


  typedef FT_Error
  (*FTDriver_setCharSizes)( FT_Size     size,
                            FT_F26Dot6  char_width,
                            FT_F26Dot6  char_height,
                            FT_UInt     horz_resolution,
                            FT_UInt     vert_resolution );

  typedef FT_Error
  (*FTDriver_setPixelSizes)( FT_Size  size,
                             FT_UInt  pixel_width,
                             FT_UInt  pixel_height );

  typedef FT_Error
  (*FTDriver_loadGlyph)( FT_GlyphSlot  slot,
                         FT_Size       size,
                         FT_UInt       glyph_index,
                         FT_Int        load_flags );


  typedef FT_UInt
  (*FTDriver_getCharIndex)( FT_CharMap  charmap,
                            FT_Long     charcode );

  typedef FT_Error
  (*FTDriver_getKerning)( FT_Face      face,
                          FT_UInt      left_glyph,
                          FT_UInt      right_glyph,
                          FT_Vector*   kerning );


  typedef FT_Error
  (*FTDriver_attachFile)( FT_Face    face,
                          FT_Stream  stream );


  typedef FT_Error
  (*FTDriver_getAdvances)( FT_Face     face,
                           FT_UInt     first,
                           FT_UInt     count,
                           FT_Bool     vertical,
                           FT_UShort*  advances );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Driver_Class                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The font driver class.  This structure mostly contains pointers to */
  /*    driver methods.                                                    */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root             :: The parent module.                             */
  /*                                                                       */
  /*    face_object_size :: The size of a face object in bytes.            */
  /*                                                                       */
  /*    size_object_size :: The size of a size object in bytes.            */
  /*                                                                       */
  /*    slot_object_size :: The size of a glyph object in bytes.           */
  /*                                                                       */
  /*    init_face        :: The format-specific face constructor.          */
  /*                                                                       */
  /*    done_face        :: The format-specific face destructor.           */
  /*                                                                       */
  /*    init_size        :: The format-specific size constructor.          */
  /*                                                                       */
  /*    done_size        :: The format-specific size destructor.           */
  /*                                                                       */
  /*    init_slot        :: The format-specific slot constructor.          */
  /*                                                                       */
  /*    done_slot        :: The format-specific slot destructor.           */
  /*                                                                       */
  /*    set_char_sizes   :: A handle to a function used to set the new     */
  /*                        character size in points + resolution.  Can be */
  /*                        set to 0 to indicate default behaviour.        */
  /*                                                                       */
  /*    set_pixel_sizes  :: A handle to a function used to set the new     */
  /*                        character size in pixels.  Can be set to 0 to  */
  /*                        indicate default behaviour.                    */
  /*                                                                       */
  /*    load_glyph       :: A function handle to load a given glyph image  */
  /*                        in a slot.  This field is mandatory!           */
  /*                                                                       */
  /*    get_char_index   :: A function handle to return the glyph index of */
  /*                        a given character for a given charmap.  This   */
  /*                        field is mandatory!                            */
  /*                                                                       */
  /*    get_kerning      :: A function handle to return the unscaled       */
  /*                        kerning for a given pair of glyphs.  Can be    */
  /*                        set to 0 if the format doesn't support         */
  /*                        kerning.                                       */
  /*                                                                       */
  /*    attach_file      :: This function handle is used to read           */
  /*                        additional data for a face from another        */
  /*                        file/stream.  For example, this can be used to */
  /*                        add data from AFM or PFM files on a Type 1     */
  /*                        face, or a CIDMap on a CID-keyed face.         */
  /*                                                                       */
  /*    get_advances     :: A function handle used to return the advances  */
  /*                        of 'count' glyphs, starting at `index'.  the   */
  /*                        `vertical' flags must be set when vertical     */
  /*                        advances are queried.  The advances buffer is  */
  /*                        caller-allocated.                              */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Most function pointers, with the exception of `load_glyph' and     */
  /*    `get_char_index' can be set to 0 to indicate a default behaviour.  */
  /*                                                                       */
  typedef struct  FT_Driver_Class_
  {
    FT_Module_Class         root;

    FT_Int                  face_object_size;
    FT_Int                  size_object_size;
    FT_Int                  slot_object_size;

    FTDriver_initFace       init_face;
    FTDriver_doneFace       done_face;

    FTDriver_initSize       init_size;
    FTDriver_doneSize       done_size;

    FTDriver_initGlyphSlot  init_slot;
    FTDriver_doneGlyphSlot  done_slot;

    FTDriver_setCharSizes   set_char_sizes;
    FTDriver_setPixelSizes  set_pixel_sizes;

    FTDriver_loadGlyph      load_glyph;
    FTDriver_getCharIndex   get_char_index;

    FTDriver_getKerning     get_kerning;
    FTDriver_attachFile     attach_file;

    FTDriver_getAdvances    get_advances;

  } FT_Driver_Class;


FT_END_HEADER

#endif /* __FTDRIVER_H__ */


/* END */
