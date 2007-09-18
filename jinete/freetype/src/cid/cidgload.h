/***************************************************************************/
/*                                                                         */
/*  cidgload.h                                                             */
/*                                                                         */
/*    OpenType Glyph Loader (specification).                               */
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


#ifndef __CIDGLOAD_H__
#define __CIDGLOAD_H__


#include <ft2build.h>
#include "cidobjs.h"


FT_BEGIN_HEADER


#if 0

  /* Compute the maximum advance width of a font through quick parsing */
  FT_LOCAL FT_Error
  CID_Compute_Max_Advance( CID_Face  face,
                           FT_Int*   max_advance );

#endif /* 0 */

  FT_LOCAL FT_Error
  CID_Load_Glyph( CID_GlyphSlot  glyph,
                  CID_Size       size,
                  FT_Int         glyph_index,
                  FT_Int         load_flags );


FT_END_HEADER

#endif /* __CIDGLOAD_H__ */


/* END */
