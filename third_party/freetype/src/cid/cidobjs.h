/***************************************************************************/
/*                                                                         */
/*  cidobjs.h                                                              */
/*                                                                         */
/*    CID objects manager (specification).                                 */
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


#ifndef __CIDOBJS_H__
#define __CIDOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  /* The following structures must be defined by the hinter */
  typedef struct CID_Size_Hints_   CID_Size_Hints;
  typedef struct CID_Glyph_Hints_  CID_Glyph_Hints;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_Driver                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 driver object.                                */
  /*                                                                       */
  typedef struct CID_DriverRec_*  CID_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_Size                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 size object.                                  */
  /*                                                                       */
  typedef struct CID_SizeRec_*  CID_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_GlyphSlot                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 glyph slot object.                            */
  /*                                                                       */
  typedef struct CID_GlyphSlotRec_*  CID_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    CID_CharMap                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a Type 1 character mapping object.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The Type 1 format doesn't use a charmap but an encoding table.     */
  /*    The driver is responsible for making up charmap objects            */
  /*    corresponding to these tables.                                     */
  /*                                                                       */
  typedef struct CID_CharMapRec_*  CID_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /* HERE BEGINS THE TYPE 1 SPECIFIC STUFF                                 */
  /*                                                                       */
  /*************************************************************************/


  typedef struct  CID_SizeRec_
  {
    FT_SizeRec  root;
    FT_Bool     valid;

  } CID_SizeRec;


  typedef struct  CID_GlyphSlotRec_
  {
    FT_GlyphSlotRec  root;

    FT_Bool          hint;
    FT_Bool          scaled;

    FT_Fixed         x_scale;
    FT_Fixed         y_scale;

  } CID_GlyphSlotRec;


  FT_LOCAL void
  CID_GlyphSlot_Done( CID_GlyphSlot  slot );

  FT_LOCAL FT_Error
  CID_GlyphSlot_Init( CID_GlyphSlot   slot );


  FT_LOCAL void
  CID_Size_Done( CID_Size  size );


  FT_LOCAL FT_Error
  CID_Size_Init( CID_Size  size );


  FT_LOCAL FT_Error
  CID_Size_Reset( CID_Size  size );


  FT_LOCAL FT_Error
  CID_Face_Init( FT_Stream      stream,
                 CID_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params );


  FT_LOCAL void
  CID_Face_Done( CID_Face  face );


  FT_LOCAL FT_Error
  CID_Driver_Init( CID_Driver  driver );


  FT_LOCAL void
  CID_Driver_Done( CID_Driver  driver );


FT_END_HEADER

#endif /* __CIDOBJS_H__ */


/* END */
