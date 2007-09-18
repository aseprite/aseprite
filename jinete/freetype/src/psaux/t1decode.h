/***************************************************************************/
/*                                                                         */
/*  t1decode.h                                                             */
/*                                                                         */
/*    PostScript Type 1 decoding routines (specification).                 */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __T1DECODE_H__
#define __T1DECODE_H__


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_TYPE1_TYPES_H


FT_BEGIN_HEADER


  FT_CALLBACK_TABLE
  const T1_Decoder_Funcs  t1_decoder_funcs;


  FT_LOCAL FT_Error
  T1_Decoder_Parse_Glyph( T1_Decoder*  decoder,
                          FT_UInt      glyph_index );

  FT_LOCAL FT_Error
  T1_Decoder_Parse_Charstrings( T1_Decoder*  decoder,
                                FT_Byte*     base,
                                FT_UInt      len );

  FT_LOCAL FT_Error
  T1_Decoder_Init( T1_Decoder*          decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   T1_Blend*            blend,
                   FT_Bool              hinting,
                   T1_Decoder_Callback  parse_glyph );

  FT_LOCAL void
  T1_Decoder_Done( T1_Decoder*  decoder );


FT_END_HEADER

#endif /* __T1DECODE_H__ */


/* END */
