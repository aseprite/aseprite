/***************************************************************************/
/*                                                                         */
/*  ftsynth.h                                                              */
/*                                                                         */
/*    FreeType synthesizing code for emboldening and slanting              */
/*    (specification).                                                     */
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


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*********                                                       *********/
  /*********        WARNING, THIS IS ALPHA CODE, THIS API          *********/
  /*********    IS DUE TO CHANGE UNTIL STRICTLY NOTIFIED BY THE    *********/
  /*********            FREETYPE DEVELOPMENT TEAM                  *********/
  /*********                                                       *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#ifndef __FTSYNTH_H__
#define __FTSYNTH_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /* This code is completely experimental -- use with care! */
  /* It will probably be completely rewritten in the future */
  /* or even integrated into the library.                   */
  FT_EXPORT( FT_Error )
  FT_Outline_Embolden( FT_GlyphSlot  original,
                       FT_Outline*   outline,
                       FT_Pos*       advance );

  FT_EXPORT( FT_Error )
  FT_Outline_Oblique( FT_GlyphSlot  original,
                      FT_Outline*   outline,
                      FT_Pos*       advance );


FT_END_HEADER

#endif /* __FTSYNTH_H__ */


/* END */
