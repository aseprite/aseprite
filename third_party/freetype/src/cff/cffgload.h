/***************************************************************************/
/*                                                                         */
/*  cffgload.h                                                             */
/*                                                                         */
/*    OpenType Glyph Loader (specification).                               */
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


#ifndef __CFFGLOAD_H__
#define __CFFGLOAD_H__


#include <ft2build.h>
#include FT_FREETYPE_H
#include "cffobjs.h"


FT_BEGIN_HEADER


#define CFF_MAX_OPERANDS     48
#define CFF_MAX_SUBRS_CALLS  32


  /*************************************************************************/
  /*                                                                       */
  /* <Structure>                                                           */
  /*    CFF_Builder                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*     A structure used during glyph loading to store its outline.       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory       :: The current memory object.                         */
  /*                                                                       */
  /*    face         :: The current face object.                           */
  /*                                                                       */
  /*    glyph        :: The current glyph slot.                            */
  /*                                                                       */
  /*    current      :: The current glyph outline.                         */
  /*                                                                       */
  /*    base         :: The base glyph outline.                            */
  /*                                                                       */
  /*    max_points   :: maximum points in builder outline                  */
  /*                                                                       */
  /*    max_contours :: Maximal number of contours in builder outline.     */
  /*                                                                       */
  /*    last         :: The last point position.                           */
  /*                                                                       */
  /*    scale_x      :: The horizontal scale (FUnits to sub-pixels).       */
  /*                                                                       */
  /*    scale_y      :: The vertical scale (FUnits to sub-pixels).         */
  /*                                                                       */
  /*    pos_x        :: The horizontal translation (if composite glyph).   */
  /*                                                                       */
  /*    pos_y        :: The vertical translation (if composite glyph).     */
  /*                                                                       */
  /*    left_bearing :: The left side bearing point.                       */
  /*                                                                       */
  /*    advance      :: The horizontal advance vector.                     */
  /*                                                                       */
  /*    bbox         :: Unused.                                            */
  /*                                                                       */
  /*    path_begun   :: A flag which indicates that a new path has begun.  */
  /*                                                                       */
  /*    load_points  :: If this flag is not set, no points are loaded.     */
  /*                                                                       */
  /*    no_recurse   :: Set but not used.                                  */
  /*                                                                       */
  /*    error        :: An error code that is only used to report memory   */
  /*                    allocation problems.                               */
  /*                                                                       */
  /*    metrics_only :: A boolean indicating that we only want to compute  */
  /*                    the metrics of a given glyph, not load all of its  */
  /*                    points.                                            */
  /*                                                                       */
  typedef struct  CFF_Builder_
  {
    FT_Memory         memory;
    TT_Face           face;
    CFF_GlyphSlot     glyph;
    FT_GlyphLoader*   loader;
    FT_Outline*       base;
    FT_Outline*       current;

    FT_Vector         last;

    FT_Fixed          scale_x;
    FT_Fixed          scale_y;

    FT_Pos            pos_x;
    FT_Pos            pos_y;

    FT_Vector         left_bearing;
    FT_Vector         advance;

    FT_BBox           bbox;          /* bounding box */
    FT_Bool           path_begun;
    FT_Bool           load_points;
    FT_Bool           no_recurse;

    FT_Error          error;         /* only used for memory errors */
    FT_Bool           metrics_only;

    void*             hints_funcs;    /* hinter-specific */
    void*             hints_globals;  /* hinter-specific */

  } CFF_Builder;


  /* execution context charstring zone */

  typedef struct  CFF_Decoder_Zone_
  {
    FT_Byte*  base;
    FT_Byte*  limit;
    FT_Byte*  cursor;

  } CFF_Decoder_Zone;


  typedef struct  CFF_Decoder_
  {
    CFF_Builder        builder;
    CFF_Font*          cff;

    FT_Fixed           stack[CFF_MAX_OPERANDS + 1];
    FT_Fixed*          top;

    CFF_Decoder_Zone   zones[CFF_MAX_SUBRS_CALLS + 1];
    CFF_Decoder_Zone*  zone;

    FT_Int             flex_state;
    FT_Int             num_flex_vectors;
    FT_Vector          flex_vectors[7];

    FT_Pos             glyph_width;
    FT_Pos             nominal_width;

    FT_Bool            read_width;
    FT_Int             num_hints;
    FT_Fixed*          buildchar;
    FT_Int             len_buildchar;

    FT_UInt            num_locals;
    FT_UInt            num_globals;

    FT_Int             locals_bias;
    FT_Int             globals_bias;

    FT_Byte**          locals;
    FT_Byte**          globals;

    FT_Byte**          glyph_names;   /* for pure CFF fonts only  */
    FT_UInt            num_glyphs;    /* number of glyphs in font */

  } CFF_Decoder;


  FT_LOCAL void
  CFF_Init_Decoder( CFF_Decoder*   decoder,
                    TT_Face        face,
                    CFF_Size       size,
                    CFF_GlyphSlot  slot,
                    FT_Bool        hinting );

  FT_LOCAL void
  CFF_Prepare_Decoder( CFF_Decoder*  decoder,
                       FT_UInt       glyph_index );

#if 0  /* unused until we support pure CFF fonts */

  /* Compute the maximum advance width of a font through quick parsing */
  FT_LOCAL FT_Error
  CFF_Compute_Max_Advance( TT_Face  face,
                           FT_Int*  max_advance );

#endif /* 0 */

  FT_LOCAL FT_Error
  CFF_Parse_CharStrings( CFF_Decoder*  decoder,
                         FT_Byte*      charstring_base,
                         FT_Int        charstring_len );

  FT_LOCAL FT_Error
  CFF_Load_Glyph( CFF_GlyphSlot  glyph,
                  CFF_Size       size,
                  FT_Int         glyph_index,
                  FT_Int         load_flags );


FT_END_HEADER

#endif /* __CFFGLOAD_H__ */


/* END */
