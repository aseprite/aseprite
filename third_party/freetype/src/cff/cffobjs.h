/***************************************************************************/
/*                                                                         */
/*  cffobjs.h                                                              */
/*                                                                         */
/*    OpenType objects manager (specification).                            */
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


#ifndef __CFFOBJS_H__
#define __CFFOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_CFF_TYPES_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CFF_Driver                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType driver object.                             */
  /*                                                                       */
  typedef struct CFF_DriverRec_*  CFF_Driver;

  typedef TT_Face  CFF_Face;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CFF_Size                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType size object.                               */
  /*                                                                       */
  typedef FT_Size  CFF_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CFF_GlyphSlot                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an OpenType glyph slot object.                         */
  /*                                                                       */
  typedef struct  CFF_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } CFF_GlyphSlotRec, *CFF_GlyphSlot;



  /*************************************************************************/
  /*                                                                       */
  /* Subglyph transformation record.                                       */
  /*                                                                       */
  typedef struct  CFF_Transform_
  {
    FT_Fixed    xx, xy;     /* transformation matrix coefficients */
    FT_Fixed    yx, yy;
    FT_F26Dot6  ox, oy;     /* offsets        */

  } CFF_Transform;


  /* this is only used in the case of a pure CFF font with no charmap */
  typedef struct  CFF_CharMapRec_
  {
    TT_CharMapRec  root;
    PS_Unicodes    unicodes;

  } CFF_CharMapRec, *CFF_CharMap;


  /***********************************************************************/
  /*                                                                     */
  /* TrueType driver class.                                              */
  /*                                                                     */
  typedef struct  CFF_DriverRec_
  {
    FT_DriverRec  root;
    void*         extension_component;

  } CFF_DriverRec;


  FT_LOCAL FT_Error
  CFF_Size_Init( CFF_Size  size );

  FT_LOCAL void
  CFF_Size_Done( CFF_Size  size );

  FT_LOCAL FT_Error
  CFF_Size_Reset( CFF_Size  size );

  FT_LOCAL void
  CFF_GlyphSlot_Done( CFF_GlyphSlot  slot );

  FT_LOCAL FT_Error
  CFF_GlyphSlot_Init( CFF_GlyphSlot   slot );

  /*************************************************************************/
  /*                                                                       */
  /* Face functions                                                        */
  /*                                                                       */
  FT_LOCAL FT_Error
  CFF_Face_Init( FT_Stream      stream,
                 CFF_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params );

  FT_LOCAL void
  CFF_Face_Done( CFF_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* Driver functions                                                      */
  /*                                                                       */
  FT_LOCAL FT_Error
  CFF_Driver_Init( CFF_Driver  driver );

  FT_LOCAL void
  CFF_Driver_Done( CFF_Driver  driver );


FT_END_HEADER

#endif /* __CFFOBJS_H__ */


/* END */
