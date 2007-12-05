/***************************************************************************/
/*                                                                         */
/*  cfftypes.h                                                             */
/*                                                                         */
/*    Basic OpenType/CFF type definitions and interface (specification     */
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


#ifndef __CFFTYPES_H__
#define __CFFTYPES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    CFF_Index                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a CFF Index table.                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    stream      :: The source input stream.                            */
  /*                                                                       */
  /*    count       :: The number of elements in the index.                */
  /*                                                                       */
  /*    off_size    :: The size in bytes of object offsets in index.       */
  /*                                                                       */
  /*    data_offset :: The position of first data byte in the index's      */
  /*                   bytes.                                              */
  /*                                                                       */
  /*    offsets     :: A table of element offsets in the index.            */
  /*                                                                       */
  /*    bytes       :: If the index is loaded in memory, its bytes.        */
  /*                                                                       */
  typedef struct  CFF_Index_
  {
    FT_Stream  stream;
    FT_UInt    count;
    FT_Byte    off_size;
    FT_ULong   data_offset;

    FT_ULong*  offsets;
    FT_Byte*   bytes;

  } CFF_Index;


  typedef struct  CFF_Encoding_
  {
    FT_UInt     format;
    FT_ULong    offset;

    FT_UShort*  sids;
    FT_UShort*  codes;

  } CFF_Encoding;


  typedef struct  CFF_Charset_
  {

    FT_UInt     format;
    FT_ULong    offset;

    FT_UShort*  sids;

  } CFF_Charset;


  typedef struct  CFF_Font_Dict_
  {
    FT_UInt    version;
    FT_UInt    notice;
    FT_UInt    copyright;
    FT_UInt    full_name;
    FT_UInt    family_name;
    FT_UInt    weight;
    FT_Bool    is_fixed_pitch;
    FT_Fixed   italic_angle;
    FT_Pos     underline_position;
    FT_Pos     underline_thickness;
    FT_Int     paint_type;
    FT_Int     charstring_type;
    FT_Matrix  font_matrix;
    FT_UShort  units_per_em;
    FT_Vector  font_offset;
    FT_ULong   unique_id;
    FT_BBox    font_bbox;
    FT_Pos     stroke_width;
    FT_ULong   charset_offset;
    FT_ULong   encoding_offset;
    FT_ULong   charstrings_offset;
    FT_ULong   private_offset;
    FT_ULong   private_size;
    FT_Long    synthetic_base;
    FT_UInt    embedded_postscript;
    FT_UInt    base_font_name;
    FT_UInt    postscript;

    /* these should only be used for the top-level font dictionary */
    FT_UInt    cid_registry;
    FT_UInt    cid_ordering;
    FT_ULong   cid_supplement;

    FT_Long    cid_font_version;
    FT_Long    cid_font_revision;
    FT_Long    cid_font_type;
    FT_Long    cid_count;
    FT_ULong   cid_uid_base;
    FT_ULong   cid_fd_array_offset;
    FT_ULong   cid_fd_select_offset;
    FT_UInt    cid_font_name;

  } CFF_Font_Dict;


  typedef struct  CFF_Private_
  {
    FT_Byte   num_blue_values;
    FT_Byte   num_other_blues;
    FT_Byte   num_family_blues;
    FT_Byte   num_family_other_blues;

    FT_Pos    blue_values[14];
    FT_Pos    other_blues[10];
    FT_Pos    family_blues[14];
    FT_Pos    family_other_blues[10];

    FT_Fixed  blue_scale;
    FT_Pos    blue_shift;
    FT_Pos    blue_fuzz;
    FT_Pos    standard_width;
    FT_Pos    standard_height;

    FT_Byte   num_snap_widths;
    FT_Byte   num_snap_heights;
    FT_Pos    snap_widths[13];
    FT_Pos    snap_heights[13];
    FT_Bool   force_bold;
    FT_Fixed  force_bold_threshold;
    FT_Int    lenIV;
    FT_Int    language_group;
    FT_Fixed  expansion_factor;
    FT_Long   initial_random_seed;
    FT_ULong  local_subrs_offset;
    FT_Pos    default_width;
    FT_Pos    nominal_width;

  } CFF_Private;


  typedef struct  CFF_FD_Select_
  {
    FT_Byte   format;
    FT_UInt   range_count;

    /* that's the table, taken from the file `as is' */
    FT_Byte*  data;
    FT_UInt   data_size;

    /* small cache for format 3 only */
    FT_UInt   cache_first;
    FT_UInt   cache_count;
    FT_Byte   cache_fd;

  } CFF_FD_Select;


  /* A SubFont packs a font dict and a private dict together. They are */
  /* needed to support CID-keyed CFF fonts.                            */
  typedef struct  CFF_SubFont_
  {
    CFF_Font_Dict  font_dict;
    CFF_Private    private_dict;

    CFF_Index      local_subrs_index;
    FT_UInt        num_local_subrs;
    FT_Byte**      local_subrs;

  } CFF_SubFont;


  /* maximum number of sub-fonts in a CID-keyed file */
#define CFF_MAX_CID_FONTS  16


  typedef struct  CFF_Font_
  {
    FT_Stream      stream;
    FT_Memory      memory;
    FT_UInt        num_faces;
    FT_UInt        num_glyphs;

    FT_Byte        version_major;
    FT_Byte        version_minor;
    FT_Byte        header_size;
    FT_Byte        absolute_offsize;


    CFF_Index      name_index;
    CFF_Index      top_dict_index;
    CFF_Index      string_index;
    CFF_Index      global_subrs_index;

    CFF_Encoding   encoding;
    CFF_Charset    charset;

    CFF_Index      charstrings_index;
    CFF_Index      font_dict_index;
    CFF_Index      private_index;
    CFF_Index      local_subrs_index;

    FT_String*     font_name;
    FT_UInt        num_global_subrs;
    FT_Byte**      global_subrs;

    CFF_SubFont    top_font;
    FT_UInt        num_subfonts;
    CFF_SubFont*   subfonts[CFF_MAX_CID_FONTS];

    CFF_FD_Select  fd_select;

    /* interface to PostScript hinter */
    void*          pshinter;

  } CFF_Font;


FT_END_HEADER

#endif /* __CFFTYPES_H__ */


/* END */
