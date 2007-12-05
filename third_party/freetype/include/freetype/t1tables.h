/***************************************************************************/
/*                                                                         */
/*  t1tables.h                                                             */
/*                                                                         */
/*    Basic Type 1/Type 2 tables definitions and interface (specification  */
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


#ifndef __T1TABLES_H__
#define __T1TABLES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    type1_tables                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Type 1 Tables                                                      */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Type 1 (PostScript) specific font tables.                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the definition of Type 1-specific tables,    */
  /*    including structures related to other PostScript font formats.     */
  /*                                                                       */
  /*************************************************************************/


  /* Note that we separate font data in T1_FontInfo and T1_Private */
  /* structures in order to support Multiple Master fonts.         */


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_FontInfo                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a Type1/Type2 FontInfo dictionary.  Note */
  /*    that for Multiple Master fonts, each instance has its own          */
  /*    FontInfo.                                                          */
  /*                                                                       */
  typedef struct  T1_FontInfo
  {
    FT_String*  version;
    FT_String*  notice;
    FT_String*  full_name;
    FT_String*  family_name;
    FT_String*  weight;
    FT_Long     italic_angle;
    FT_Bool     is_fixed_pitch;
    FT_Short    underline_position;
    FT_UShort   underline_thickness;

  } T1_FontInfo;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Private                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a Type1/Type2 FontInfo dictionary.  Note */
  /*    that for Multiple Master fonts, each instance has its own Private  */
  /*    dict.                                                              */
  /*                                                                       */
  typedef struct  T1_Private
  {
    FT_Int     unique_id;
    FT_Int     lenIV;

    FT_Byte    num_blue_values;
    FT_Byte    num_other_blues;
    FT_Byte    num_family_blues;
    FT_Byte    num_family_other_blues;

    FT_Short   blue_values[14];
    FT_Short   other_blues[10];

    FT_Short   family_blues      [14];
    FT_Short   family_other_blues[10];

    FT_Fixed   blue_scale;
    FT_Int     blue_shift;
    FT_Int     blue_fuzz;

    FT_UShort  standard_width[1];
    FT_UShort  standard_height[1];

    FT_Byte    num_snap_widths;
    FT_Byte    num_snap_heights;
    FT_Bool    force_bold;
    FT_Bool    round_stem_up;

    FT_Short   snap_widths [13];  /* including std width  */
    FT_Short   snap_heights[13];  /* including std height */

    FT_Long    language_group;
    FT_Long    password;

    FT_Short   min_feature[2];

  } T1_Private;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    T1_Blend_Flags                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A set of flags used to indicate which fields are present in a      */
  /*    given blen dictionary (font info or private).  Used to support     */
  /*    Multiple Masters fonts.                                            */
  /*                                                                       */
  typedef enum
  {
    /*# required fields in a FontInfo blend dictionary */
    t1_blend_underline_position = 0,
    t1_blend_underline_thickness,
    t1_blend_italic_angle,

    /*# required fields in a Private blend dictionary */
    t1_blend_blue_values,
    t1_blend_other_blues,
    t1_blend_standard_width,
    t1_blend_standard_height,
    t1_blend_stem_snap_widths,
    t1_blend_stem_snap_heights,
    t1_blend_blue_scale,
    t1_blend_blue_shift,
    t1_blend_family_blues,
    t1_blend_family_other_blues,
    t1_blend_force_bold,

    /*# never remove */
    t1_blend_max

  } T1_Blend_Flags;


  /* maximum number of Multiple Masters designs, as defined in the spec */
#define T1_MAX_MM_DESIGNS     16

  /* maximum number of Multiple Masters axes, as defined in the spec */
#define T1_MAX_MM_AXIS         4

  /* maximum number of elements in a design map */
#define T1_MAX_MM_MAP_POINTS  20


  /* this structure is used to store the BlendDesignMap entry for an axis */
  typedef struct  T1_DesignMap_
  {
    FT_Byte    num_points;
    FT_Fixed*  design_points;
    FT_Fixed*  blend_points;

  } T1_DesignMap;


  typedef struct  T1_Blend_
  {
    FT_UInt       num_designs;
    FT_UInt       num_axis;

    FT_String*    axis_names[T1_MAX_MM_AXIS];
    FT_Fixed*     design_pos[T1_MAX_MM_DESIGNS];
    T1_DesignMap  design_map[T1_MAX_MM_AXIS];

    FT_Fixed*     weight_vector;
    FT_Fixed*     default_weight_vector;

    T1_FontInfo*  font_infos[T1_MAX_MM_DESIGNS + 1];
    T1_Private*   privates  [T1_MAX_MM_DESIGNS + 1];

    FT_ULong      blend_bitflags;

  } T1_Blend;


  typedef struct  CID_FontDict_
  {
    T1_Private  private_dict;

    FT_UInt     len_buildchar;
    FT_Fixed    forcebold_threshold;
    FT_Pos      stroke_width;
    FT_Fixed    expansion_factor;

    FT_Byte     paint_type;
    FT_Byte     font_type;
    FT_Matrix   font_matrix;
    FT_Vector   font_offset;

    FT_UInt     num_subrs;
    FT_ULong    subrmap_offset;
    FT_Int      sd_bytes;

  } CID_FontDict;


  typedef struct  CID_Info_
  {
    FT_String*     cid_font_name;
    FT_Fixed       cid_version;
    FT_Int         cid_font_type;

    FT_String*     registry;
    FT_String*     ordering;
    FT_Int         supplement;

    T1_FontInfo    font_info;
    FT_BBox        font_bbox;
    FT_ULong       uid_base;

    FT_Int         num_xuid;
    FT_ULong       xuid[16];


    FT_ULong       cidmap_offset;
    FT_Int         fd_bytes;
    FT_Int         gd_bytes;
    FT_ULong       cid_count;

    FT_Int         num_dicts;
    CID_FontDict*  font_dicts;

    FT_ULong       data_offset;

  } CID_Info;


  /* */


FT_END_HEADER

#endif /* __T1TABLES_H__ */


/* END */
