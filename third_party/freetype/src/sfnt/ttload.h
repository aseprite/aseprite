/***************************************************************************/
/*                                                                         */
/*  ttload.h                                                               */
/*                                                                         */
/*    Load the basic TrueType tables, i.e., tables that can be either in   */
/*    TTF or OTF fonts (specification).                                    */
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


#ifndef __TTLOAD_H__
#define __TTLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL TT_Table*
  TT_LookUp_Table( TT_Face   face,
                   FT_ULong  tag );

  FT_LOCAL FT_Error
  TT_Goto_Table( TT_Face    face,
                 FT_ULong   tag,
                 FT_Stream  stream,
                 FT_ULong*  length );


  FT_LOCAL FT_Error
  TT_Load_SFNT_Header( TT_Face       face,
                       FT_Stream     stream,
                       FT_Long       face_index,
                       SFNT_Header*  sfnt );

  FT_LOCAL FT_Error
  TT_Load_Directory( TT_Face       face,
                     FT_Stream     stream,
                     SFNT_Header*  sfnt );

  FT_LOCAL FT_Error
  TT_Load_Any( TT_Face    face,
               FT_ULong   tag,
               FT_Long    offset,
               FT_Byte*   buffer,
               FT_ULong*  length );


  FT_LOCAL FT_Error
  TT_Load_Header( TT_Face    face,
                  FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_Metrics_Header( TT_Face    face,
                          FT_Stream  stream,
                          FT_Bool    vertical );


  FT_LOCAL FT_Error
  TT_Load_CMap( TT_Face    face,
                FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_MaxProfile( TT_Face    face,
                      FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_Names( TT_Face    face,
                 FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_OS2( TT_Face    face,
               FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_PostScript( TT_Face    face,
                      FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_Hdmx( TT_Face    face,
                FT_Stream  stream );

  FT_LOCAL FT_Error
  TT_Load_PCLT( TT_Face    face,
                FT_Stream  stream );

  FT_LOCAL void
  TT_Free_Names( TT_Face  face );


  FT_LOCAL void
  TT_Free_Hdmx ( TT_Face  face );


  FT_LOCAL FT_Error
  TT_Load_Kern( TT_Face    face,
                FT_Stream  stream );


  FT_LOCAL FT_Error
  TT_Load_Gasp( TT_Face    face,
                FT_Stream  stream );

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

  FT_LOCAL FT_Error
  TT_Load_Bitmap_Header( TT_Face    face,
                         FT_Stream  stream );

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */


FT_END_HEADER

#endif /* __TTLOAD_H__ */


/* END */
