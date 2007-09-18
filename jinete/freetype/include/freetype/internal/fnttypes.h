/***************************************************************************/
/*                                                                         */
/*  fnttypes.h                                                             */
/*                                                                         */
/*    Basic Windows FNT/FON type definitions and interface (specification  */
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


#ifndef __FNTTYPES_H__
#define __FNTTYPES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  typedef struct  WinMZ_Header_
  {
    FT_UShort  magic;
    /* skipped content */
    FT_UShort  lfanew;

  } WinMZ_Header;


  typedef struct  WinNE_Header_
  {
    FT_UShort  magic;
    /* skipped content */
    FT_UShort  resource_tab_offset;
    FT_UShort  rname_tab_offset;

  } WinNE_Header;


  typedef struct  WinNameInfo_
  {
    FT_UShort  offset;
    FT_UShort  length;
    FT_UShort  flags;
    FT_UShort  id;
    FT_UShort  handle;
    FT_UShort  usage;

  } WinNameInfo;


  typedef struct  WinResourceInfo_
  {
    FT_UShort  type_id;
    FT_UShort  count;

  } WinResourceInfo;


#define WINFNT_MZ_MAGIC  0x5A4D
#define WINFNT_NE_MAGIC  0x454E


  typedef struct  WinFNT_Header_
  {
    FT_UShort  version;
    FT_ULong   file_size;
    FT_Byte    copyright[60];
    FT_UShort  file_type;
    FT_UShort  nominal_point_size;
    FT_UShort  vertical_resolution;
    FT_UShort  horizontal_resolution;
    FT_UShort  ascent;
    FT_UShort  internal_leading;
    FT_UShort  external_leading;
    FT_Byte    italic;
    FT_Byte    underline;
    FT_Byte    strike_out;
    FT_UShort  weight;
    FT_Byte    charset;
    FT_UShort  pixel_width;
    FT_UShort  pixel_height;
    FT_Byte    pitch_and_family;
    FT_UShort  avg_width;
    FT_UShort  max_width;
    FT_Byte    first_char;
    FT_Byte    last_char;
    FT_Byte    default_char;
    FT_Byte    break_char;
    FT_UShort  bytes_per_row;
    FT_ULong   device_offset;
    FT_ULong   face_name_offset;
    FT_ULong   bits_pointer;
    FT_ULong   bits_offset;
    FT_Byte    reserved;
    FT_ULong   flags;
    FT_UShort  A_space;
    FT_UShort  B_space;
    FT_UShort  C_space;
    FT_UShort  color_table_offset;
    FT_Byte    reserved2[4];

  } WinFNT_Header;


  typedef struct  FNT_Font_
  {
    FT_ULong       offset;
    FT_Int         size_shift;

    WinFNT_Header  header;

    FT_Byte*       fnt_frame;
    FT_ULong       fnt_size;

  } FNT_Font;


  typedef struct  FNT_SizeRec_
  {
    FT_SizeRec  root;
    FNT_Font*   font;

  } FNT_SizeRec, *FNT_Size;


  typedef struct  FNT_FaceRec_
  {
    FT_FaceRec     root;

    FT_UInt        num_fonts;
    FNT_Font*      fonts;

    FT_CharMap     charmap_handle;
    FT_CharMapRec  charmap;  /* a single charmap per face */

  } FNT_FaceRec, *FNT_Face;


FT_END_HEADER

#endif /* __FNTTYPES_H__ */


/* END */
