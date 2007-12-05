/***************************************************************************/
/*                                                                         */
/*  cfftoken.h                                                             */
/*                                                                         */
/*    CFF token definitions                                                */
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


#undef  FT_STRUCTURE
#define FT_STRUCTURE  CFF_Font_Dict

#undef  CFFCODE
#define CFFCODE        CFFCODE_TOPDICT

  CFF_FIELD_STRING  ( 0,     version )
  CFF_FIELD_STRING  ( 1,     notice )
  CFF_FIELD_STRING  ( 0x100, copyright )
  CFF_FIELD_STRING  ( 2,     full_name )
  CFF_FIELD_STRING  ( 3,     family_name )
  CFF_FIELD_STRING  ( 4,     weight )
  CFF_FIELD_BOOL    ( 0x101, is_fixed_pitch )
  CFF_FIELD_FIXED   ( 0x102, italic_angle )
  CFF_FIELD_NUM     ( 0x103, underline_position )
  CFF_FIELD_NUM     ( 0x104, underline_thickness )
  CFF_FIELD_NUM     ( 0x105, paint_type )
  CFF_FIELD_NUM     ( 0x106, charstring_type )
  CFF_FIELD_CALLBACK( 0x107, font_matrix )
  CFF_FIELD_NUM     ( 13,    unique_id )
  CFF_FIELD_CALLBACK( 5,     font_bbox )
  CFF_FIELD_NUM     ( 0x108, stroke_width )
  CFF_FIELD_NUM     ( 15,    charset_offset )
  CFF_FIELD_NUM     ( 16,    encoding_offset )
  CFF_FIELD_NUM     ( 17,    charstrings_offset )
  CFF_FIELD_CALLBACK( 18,    private_dict )
  CFF_FIELD_NUM     ( 0x114, synthetic_base )
  CFF_FIELD_STRING  ( 0x115, postscript )
  CFF_FIELD_STRING  ( 0x116, base_font_name )

#if 0
  CFF_FIELD_DELTA   ( 0x117, base_font_blend, 16 )
  CFF_FIELD_CALLBACK( 0x118, multiple_master )
  CFF_FIELD_CALLBACK( 0x119, blend_axit_types )
#endif

  CFF_FIELD_CALLBACK( 0x11E, cid_ros )
  CFF_FIELD_NUM     ( 0x11F, cid_font_version )
  CFF_FIELD_NUM     ( 0x120, cid_font_revision )
  CFF_FIELD_NUM     ( 0x121, cid_font_type )
  CFF_FIELD_NUM     ( 0x122, cid_count )
  CFF_FIELD_NUM     ( 0x123, cid_uid_base )
  CFF_FIELD_NUM     ( 0x124, cid_fd_array_offset )
  CFF_FIELD_NUM     ( 0x125, cid_fd_select_offset )
  CFF_FIELD_STRING  ( 0x126, cid_font_name )

#if 0
  CFF_FIELD_NUM     ( 0x127, chameleon )
#endif


#undef  FT_STRUCTURE
#define FT_STRUCTURE  CFF_Private
#undef  CFFCODE
#define CFFCODE       CFFCODE_PRIVATE

  CFF_FIELD_DELTA( 6,     blue_values, 14 )
  CFF_FIELD_DELTA( 7,     other_blues, 10 )
  CFF_FIELD_DELTA( 8,     family_blues, 14 )
  CFF_FIELD_DELTA( 9,     family_other_blues, 10 )
  CFF_FIELD_FIXED( 0x109, blue_scale )
  CFF_FIELD_NUM  ( 0x10A, blue_shift )
  CFF_FIELD_NUM  ( 0x10B, blue_fuzz )
  CFF_FIELD_NUM  ( 10,    standard_width )
  CFF_FIELD_NUM  ( 11,    standard_height )
  CFF_FIELD_DELTA( 0x10C, snap_widths, 13 )
  CFF_FIELD_DELTA( 0x10D, snap_heights, 13 )
  CFF_FIELD_BOOL ( 0x10E, force_bold )
  CFF_FIELD_FIXED( 0x10F, force_bold_threshold )
  CFF_FIELD_NUM  ( 0x110, lenIV )
  CFF_FIELD_NUM  ( 0x111, language_group )
  CFF_FIELD_FIXED( 0x112, expansion_factor )
  CFF_FIELD_NUM  ( 0x113, initial_random_seed )
  CFF_FIELD_NUM  ( 19,    local_subrs_offset )
  CFF_FIELD_NUM  ( 20,    default_width )
  CFF_FIELD_NUM  ( 21,    nominal_width )


/* END */
