/***************************************************************************/
/*                                                                         */
/*  t1types.h                                                              */
/*                                                                         */
/*    Basic Type1/Type2 type definitions and interface (specification      */
/*    only).                                                               */
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


#ifndef __T1TYPES_H__
#define __T1TYPES_H__


#include<ft2build.h>
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***                                                                   ***/
  /***              REQUIRED TYPE1/TYPE2 TABLES DEFINITIONS              ***/
  /***                                                                   ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Encoding                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure modeling a custom encoding.                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_chars  :: The number of character codes in the encoding.       */
  /*                  Usually 256.                                         */
  /*                                                                       */
  /*    code_first :: The lowest valid character code in the encoding.     */
  /*                                                                       */
  /*    code_last  :: The highest valid character code in the encoding.    */
  /*                                                                       */
  /*    char_index :: An array of corresponding glyph indices.             */
  /*                                                                       */
  /*    char_name  :: An array of corresponding glyph names.               */
  /*                                                                       */
  typedef struct  T1_Encoding_
  {
    FT_Int       num_chars;
    FT_Int       code_first;
    FT_Int       code_last;

    FT_UShort*   char_index;
    FT_String**  char_name;

  } T1_Encoding;


  typedef enum  T1_EncodingType_
  {
    t1_encoding_none = 0,
    t1_encoding_array,
    t1_encoding_standard,
    t1_encoding_expert

  } T1_EncodingType;


  typedef struct  T1_Font_
  {
    /* font info dictionary */
    T1_FontInfo      font_info;

    /* private dictionary */
    T1_Private       private_dict;

    /* top-level dictionary */
    FT_String*       font_name;

    T1_EncodingType  encoding_type;
    T1_Encoding      encoding;

    FT_Byte*         subrs_block;
    FT_Byte*         charstrings_block;
    FT_Byte*         glyph_names_block;

    FT_Int           num_subrs;
    FT_Byte**        subrs;
    FT_Int*          subrs_len;

    FT_Int           num_glyphs;
    FT_String**      glyph_names;       /* array of glyph names       */
    FT_Byte**        charstrings;       /* array of glyph charstrings */
    FT_Int*          charstrings_len;

    FT_Byte          paint_type;
    FT_Byte          font_type;
    FT_Matrix        font_matrix;
    FT_Vector        font_offset;
    FT_BBox          font_bbox;
    FT_Long          font_id;

    FT_Int           stroke_width;

  } T1_Font;


  typedef struct  CID_Subrs_
  {
    FT_UInt    num_subrs;
    FT_Byte**  code;

  } CID_Subrs;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /***                                                                   ***/
  /***                                                                   ***/
  /***                ORIGINAL T1_FACE CLASS DEFINITION                  ***/
  /***                                                                   ***/
  /***                                                                   ***/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This structure/class is defined here because it is common to the      */
  /* following formats: TTF, OpenType-TT, and OpenType-CFF.                */
  /*                                                                       */
  /* Note, however, that the classes TT_Size, TT_GlyphSlot, and TT_CharMap */
  /* are not shared between font drivers, and are thus defined normally in */
  /* `ttobjs.h'.                                                           */
  /*                                                                       */
  /*************************************************************************/

  typedef struct T1_FaceRec_*   T1_Face;
  typedef struct CID_FaceRec_*  CID_Face;


  typedef struct  T1_FaceRec_
  {
    FT_FaceRec     root;
    T1_Font        type1;
    void*          psnames;
    void*          psaux;
    void*          afm_data;
    FT_CharMapRec  charmaprecs[2];
    FT_CharMap     charmaps[2];
    PS_Unicodes    unicode_map;

    /* support for Multiple Masters fonts */
    T1_Blend*      blend;
    
    /* since FT 2.1 - interface to PostScript hinter */
    void*          pshinter;

  } T1_FaceRec;


  typedef struct  CID_FaceRec_
  {
    FT_FaceRec  root;
    void*       psnames;
    void*       psaux;
    CID_Info    cid;
    void*       afm_data;
    CID_Subrs*  subrs;
    
    /* since FT 2.1 - interface to PostScript hinter */
    void*       pshinter;

  } CID_FaceRec;


FT_END_HEADER

#endif /* __T1TYPES_H__ */


/* END */
