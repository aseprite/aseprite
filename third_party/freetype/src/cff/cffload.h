/***************************************************************************/
/*                                                                         */
/*  cffload.h                                                              */
/*                                                                         */
/*    OpenType & CFF data/program tables loader (specification).           */
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


#ifndef __CFFLOAD_H__
#define __CFFLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_CFF_TYPES_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H


FT_BEGIN_HEADER

  FT_LOCAL FT_UShort
  CFF_Get_Standard_Encoding( FT_UInt  charcode );


  FT_LOCAL FT_String*
  CFF_Get_Name( CFF_Index*  index,
                FT_UInt     element );

  FT_LOCAL FT_String*
  CFF_Get_String( CFF_Index*          index,
                  FT_UInt             sid,
                  PSNames_Interface*  interface );


  FT_LOCAL FT_Error
  CFF_Access_Element( CFF_Index*  index,
                      FT_UInt     element,
                      FT_Byte**   pbytes,
                      FT_ULong*   pbyte_len );

  FT_LOCAL void
  CFF_Forget_Element( CFF_Index*  index,
                      FT_Byte**   pbytes );


  FT_LOCAL FT_Error
  CFF_Load_Font( FT_Stream  stream,
                 FT_Int     face_index,
                 CFF_Font*  font );

  FT_LOCAL void
  CFF_Done_Font( CFF_Font*  font );


  FT_LOCAL FT_Byte
  CFF_Get_FD( CFF_FD_Select*  select,
              FT_UInt         glyph_index );


FT_END_HEADER

#endif /* __CFFLOAD_H__ */


/* END */
