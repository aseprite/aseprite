/***************************************************************************/
/*                                                                         */
/*  ttpload.h                                                              */
/*                                                                         */
/*    TrueType glyph data/program tables loader (specification).           */
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


#ifndef __TTPLOAD_H__
#define __TTPLOAD_H__


#include <ft2build.h>
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  FT_LOCAL FT_Error
  TT_Load_Locations( TT_Face    face,
                     FT_Stream  stream );

  FT_LOCAL FT_Error
  TT_Load_CVT( TT_Face    face,
               FT_Stream  stream );

  FT_LOCAL FT_Error
  TT_Load_Programs( TT_Face    face,
                    FT_Stream  stream );


FT_END_HEADER

#endif /* __TTPLOAD_H__ */


/* END */
