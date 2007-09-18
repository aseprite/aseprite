/***************************************************************************/
/*                                                                         */
/*  psglobal.h                                                             */
/*                                                                         */
/*    Global PostScript hinting structures (specification only).           */
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


#ifndef __PSGLOBAL_H__
#define __PSGLOBAL_H__


FT_BEGIN_HEADER


  /**********************************************************************/
  /**********************************************************************/
  /*****                                                            *****/
  /*****                  PUBLIC STRUCTURES & API                   *****/
  /*****                                                            *****/
  /**********************************************************************/
  /**********************************************************************/

#if 0

  /*************************************************************************/
  /*                                                                       */
  /* @constant:                                                            */
  /*    PS_GLOBALS_MAX_BLUE_ZONES                                          */
  /*                                                                       */
  /* @description:                                                         */
  /*    The maximum number of blue zones in a font global hints structure. */
  /*    See @PS_Globals_BluesRec.                                          */
  /*                                                                       */
#define PS_GLOBALS_MAX_BLUE_ZONES  16


  /*************************************************************************/
  /*                                                                       */
  /* @constant:                                                            */
  /*    PS_GLOBALS_MAX_STD_WIDTHS                                          */
  /*                                                                       */
  /* @description:                                                         */
  /*    The maximum number of standard and snap widths in either the       */
  /*    horizontal or vertical direction.  See @PS_Globals_WidthsRec.      */
  /*                                                                       */
#define PS_GLOBALS_MAX_STD_WIDTHS  16


  /*************************************************************************/
  /*                                                                       */
  /* @type:                                                                */
  /*    PS_Globals                                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*    A handle to a @PS_GlobalsRec structure used to describe the global */
  /*    hints of a given font.                                             */
  /*                                                                       */
  typedef struct PS_GlobalsRec_*  PS_Globals;


  /*************************************************************************/
  /*                                                                       */
  /* @struct:                                                              */
  /*    PS_Globals_BluesRec                                                */
  /*                                                                       */
  /* @description:                                                         */
  /*    A structure used to model the global blue zones of a given font.   */
  /*                                                                       */
  /* @fields:                                                              */
  /*   count        :: The number of blue zones.                           */
  /*                                                                       */
  /*   zones        :: An array of (count*2) coordinates describing the    */
  /*                   zones.                                              */
  /*                                                                       */
  /*   count_family :: The number of family blue zones.                    */
  /*                                                                       */
  /*   zones_family :: An array of (count_family*2) coordinates describing */
  /*                   the family blue zones.                              */
  /*                                                                       */
  /*   scale        :: The blue scale to be used (fixed float).            */
  /*                                                                       */
  /*   shift        :: The blue shift to be used.                          */
  /*                                                                       */
  /*   fuzz         :: Te blue fuzz to be used.                            */
  /*                                                                       */
  /* @note:                                                                */
  /*    Each blue zone is modeled by a (reference,overshoot) coordinate    */
  /*    pair in the table.  Zones can be placed in any order.              */
  /*                                                                       */
  typedef struct  PS_Globals_BluesRec_
  {
    FT_UInt   count;
    FT_Int16  zones[2 * PS_GLOBALS_MAX_BLUE_ZONES];

    FT_UInt   count_family;
    FT_Int16  zones_family[2 * PS_GLOBALS_MAX_BLUE_ZONES];

    FT_Fixed  scale;
    FT_Int16  shift;
    FT_Int16  fuzz;

  } PS_Globals_BluesRec, *PS_Globals_Blues;


  /*************************************************************************/
  /*                                                                       */
  /* @type:                                                                */
  /*    PS_Global_Widths                                                   */
  /*                                                                       */
  /* @description:                                                         */
  /*    A handle to a @PS_Globals_WidthsRec structure used to model the    */
  /*    global standard and snap widths in a given direction.              */
  /*                                                                       */
  typedef struct PS_Globals_WidthsRec_*  PS_Globals_Widths;


  /*************************************************************************/
  /*                                                                       */
  /* @struct:                                                              */
  /*    PS_Globals_WidthsRec                                               */
  /*                                                                       */
  /* @description:                                                         */
  /*    A structure used to model the global standard and snap widths in a */
  /*    given font.                                                        */
  /*                                                                       */
  /* @fields:                                                              */
  /*    count  :: The number of widths.                                    */
  /*                                                                       */
  /*    widths :: An array of `count' widths in font units.                */
  /*                                                                       */
  /* @note:                                                                */
  /*    `widths[0]' must be the standard width or height, while remaining  */
  /*    elements of the array are snap widths or heights.                  */
  /*                                                                       */
  typedef struct  PS_Globals_WidthsRec_
  {
    FT_UInt   count;
    FT_Int16  widths[PS_GLOBALS_MAX_STD_WIDTHS];

  } PS_Globals_WidthsRec;


  /*************************************************************************/
  /*                                                                       */
  /* @struct:                                                              */
  /*    PS_GlobalsRec                                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*    A structure used to model the global hints for a given font.       */
  /*                                                                       */
  /* @fields:                                                              */
  /*    horizontal :: The horizontal widths.                               */
  /*                                                                       */
  /*    vertical   :: The vertical heights.                                */
  /*                                                                       */
  /*    blues      :: The blue zones.                                      */
  /*                                                                       */
  typedef struct  PS_GlobalsRec_
  {
    PS_Globals_WidthsRec  horizontal;
    PS_Globals_WidthsRec  vertical;
    PS_Globals_BluesRec   blues;

  } PS_GlobalsRec;

#endif

 /* */

FT_END_HEADER

#endif /* __PS_GLOBAL_H__ */


/* END */
