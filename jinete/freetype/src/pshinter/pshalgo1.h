/***************************************************************************/
/*                                                                         */
/*  pshalgo1.h                                                             */
/*                                                                         */
/*    PostScript hinting algorithm 1 (specification).                      */
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


#ifndef __PSHALGO1_H__
#define __PSHALGO1_H__

#include "pshrec.h"

FT_BEGIN_HEADER

  typedef struct PSH1_HintRec_*  PSH1_Hint;

  typedef enum
  {
    PSH1_HINT_FLAG_GHOST  = PS_HINT_FLAG_GHOST,
    PSH1_HINT_FLAG_BOTTOM = PS_HINT_FLAG_BOTTOM,
    PSH1_HINT_FLAG_ACTIVE = 4

  } PSH1_Hint_Flags;

#define psh1_hint_is_active( x )                          \
          ( ( (x)->flags & PSH1_HINT_FLAG_ACTIVE ) != 0 )
#define psh1_hint_is_ghost( x )                          \
          ( ( (x)->flags & PSH1_HINT_FLAG_GHOST ) != 0 )

#define psh1_hint_activate( x )    (x)->flags |= PSH1_HINT_FLAG_ACTIVE
#define psh1_hint_deactivate( x )  (x)->flags &= ~PSH1_HINT_FLAG_ACTIVE

  typedef struct  PSH1_HintRec_
  {
    FT_Int     org_pos;
    FT_Int     org_len;
    FT_Pos     cur_pos;
    FT_Pos     cur_len;

    FT_UInt    flags;

    PSH1_Hint  parent;
    FT_Int     order;

  } PSH1_HintRec;


  /* this is an interpolation zone used for strong points;  */
  /* weak points are interpolated according to their strong */
  /* neighbours                                             */
  typedef struct  PSH1_ZoneRec_
  {
    FT_Fixed  scale;
    FT_Fixed  delta;
    FT_Pos    min;
    FT_Pos    max;

  } PSH1_ZoneRec, *PSH1_Zone;


  typedef struct  PSH1_Hint_TableRec_
  {
    FT_UInt        max_hints;
    FT_UInt        num_hints;
    PSH1_Hint      hints;
    PSH1_Hint*     sort;
    PSH1_Hint*     sort_global;
    FT_UInt        num_zones;
    PSH1_Zone      zones;
    PSH1_Zone      zone;
    PS_Mask_Table  hint_masks;
    PS_Mask_Table  counter_masks;

  } PSH1_Hint_TableRec, *PSH1_Hint_Table;


  extern FT_Error
  ps1_hints_apply( PS_Hints     ps_hints,
                   FT_Outline*  outline,
                   PSH_Globals  globals );


#ifdef DEBUG_HINTER
  extern  PSH1_Hint_Table  ps1_debug_hint_table;

  typedef void
  (*PSH1_HintFunc)( PSH1_Hint  hint,
                    FT_Bool    vertical );

  extern PSH1_HintFunc  ps1_debug_hint_func;
#endif

FT_END_HEADER

#endif /* __PSHALGO1_H__ */


/* END */
