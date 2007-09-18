/***************************************************************************/
/*                                                                         */
/*  pshalgo2.h                                                             */
/*                                                                         */
/*    PostScript hinting algorithm 2 (specification).                      */
/*                                                                         */
/*  Copyright 2001 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __PSHALGO2_H__
#define __PSHALGO2_H__


#include "pshrec.h"
#include "pshglob.h"
#include FT_TRIGONOMETRY_H


FT_BEGIN_HEADER


  typedef struct PSH2_HintRec_*  PSH2_Hint;

  typedef enum
  {
    PSH2_HINT_GHOST  = PS_HINT_FLAG_GHOST,
    PSH2_HINT_BOTTOM = PS_HINT_FLAG_BOTTOM,
    PSH2_HINT_ACTIVE = 4,
    PSH2_HINT_FITTED = 8

  } PSH2_Hint_Flags;


#define psh2_hint_is_active( x )  ( ( (x)->flags & PSH2_HINT_ACTIVE ) != 0 )
#define psh2_hint_is_ghost( x )   ( ( (x)->flags & PSH2_HINT_GHOST  ) != 0 )
#define psh2_hint_is_fitted( x )  ( ( (x)->flags & PSH2_HINT_FITTED ) != 0 )

#define psh2_hint_activate( x )    (x)->flags |=  PSH2_HINT_ACTIVE
#define psh2_hint_deactivate( x )  (x)->flags &= ~PSH2_HINT_ACTIVE
#define psh2_hint_set_fitted( x )  (x)->flags |=  PSH2_HINT_FITTED


  typedef struct  PSH2_HintRec_
  {
    FT_Int     org_pos;
    FT_Int     org_len;
    FT_Pos     cur_pos;
    FT_Pos     cur_len;
    FT_UInt    flags;
    PSH2_Hint  parent;
    FT_Int     order;

  } PSH2_HintRec;


  /* this is an interpolation zone used for strong points;  */
  /* weak points are interpolated according to their strong */
  /* neighbours                                             */
  typedef struct  PSH2_ZoneRec_
  {
    FT_Fixed  scale;
    FT_Fixed  delta;
    FT_Pos    min;
    FT_Pos    max;

  } PSH2_ZoneRec, *PSH2_Zone;


  typedef struct  PSH2_Hint_TableRec_
  {
    FT_UInt        max_hints;
    FT_UInt        num_hints;
    PSH2_Hint      hints;
    PSH2_Hint*     sort;
    PSH2_Hint*     sort_global;
    FT_UInt        num_zones;
    PSH2_Zone      zones;
    PSH2_Zone      zone;
    PS_Mask_Table  hint_masks;
    PS_Mask_Table  counter_masks;

  } PSH2_Hint_TableRec, *PSH2_Hint_Table;


  typedef struct PSH2_PointRec_*    PSH2_Point;
  typedef struct PSH2_ContourRec_*  PSH2_Contour;

  enum
  {
    PSH2_DIR_NONE  =  4,
    PSH2_DIR_UP    =  1,
    PSH2_DIR_DOWN  = -1,
    PSH2_DIR_LEFT  = -2,
    PSH2_DIR_RIGHT =  2
  };

  enum
  {
    PSH2_POINT_OFF    = 1,   /* point is off the curve  */
    PSH2_POINT_STRONG = 2,   /* point is strong         */
    PSH2_POINT_SMOOTH = 4,   /* point is smooth         */
    PSH2_POINT_FITTED = 8    /* point is already fitted */
  };


  typedef struct  PSH2_PointRec_
  {
    PSH2_Point    prev;
    PSH2_Point    next;
    PSH2_Contour  contour;
    FT_UInt       flags;
    FT_Char       dir_in;
    FT_Char       dir_out;
    FT_Angle      angle_in;
    FT_Angle      angle_out;
    PSH2_Hint     hint;
    FT_Pos        org_u;
    FT_Pos        cur_u;
#ifdef DEBUG_HINTER
    FT_Pos        org_x;
    FT_Pos        cur_x;
    FT_Pos        org_y;
    FT_Pos        cur_y;
    FT_UInt       flags_x;
    FT_UInt       flags_y;
#endif

  } PSH2_PointRec;


#define psh2_point_is_strong( p )   ( (p)->flags & PSH2_POINT_STRONG )
#define psh2_point_is_fitted( p )   ( (p)->flags & PSH2_POINT_FITTED )
#define psh2_point_is_smooth( p )   ( (p)->flags & PSH2_POINT_SMOOTH )

#define psh2_point_set_strong( p )  (p)->flags |= PSH2_POINT_STRONG
#define psh2_point_set_fitted( p )  (p)->flags |= PSH2_POINT_FITTED
#define psh2_point_set_smooth( p )  (p)->flags |= PSH2_POINT_SMOOTH


  typedef struct  PSH2_ContourRec_
  {
    PSH2_Point  start;
    FT_UInt     count;

  } PSH2_ContourRec;


  typedef struct  PSH2_GlyphRec_
  {
    FT_UInt             num_points;
    FT_UInt             num_contours;

    PSH2_Point          points;
    PSH2_Contour        contours;

    FT_Memory           memory;
    FT_Outline*         outline;
    PSH_Globals         globals;
    PSH2_Hint_TableRec  hint_tables[2];

    FT_Bool             vertical;
    FT_Int              major_dir;
    FT_Int              minor_dir;

  } PSH2_GlyphRec, *PSH2_Glyph;


#ifdef DEBUG_HINTER
  extern PSH2_Hint_Table  ps2_debug_hint_table;

  typedef void
  (*PSH2_HintFunc)( PSH2_Hint  hint,
                    FT_Bool    vertical );

  extern PSH2_HintFunc    ps2_debug_hint_func;

  extern PSH2_Glyph       ps2_debug_glyph;
#endif


  extern FT_Error
  ps2_hints_apply( PS_Hints     ps_hints,
                   FT_Outline*  outline,
                   PSH_Globals  globals );


FT_END_HEADER


#endif /* __PSHALGO2_H__ */


/* END */
