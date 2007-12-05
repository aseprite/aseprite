/***************************************************************************/
/*                                                                         */
/*  sfobjs.h                                                               */
/*                                                                         */
/*    SFNT object management (specification).                              */
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


#ifndef __SFOBJS_H__
#define __SFOBJS_H__


#include <ft2build.h>
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  FT_LOCAL FT_Error
  SFNT_Init_Face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params );

  FT_LOCAL FT_Error
  SFNT_Load_Face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params );

  FT_LOCAL void
  SFNT_Done_Face( TT_Face  face );


FT_END_HEADER

#endif /* __SFDRIVER_H__ */


/* END */
