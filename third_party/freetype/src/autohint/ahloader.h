/***************************************************************************/
/*                                                                         */
/*  ahloader.h                                                             */
/*                                                                         */
/*    Glyph loader for the auto-hinting module (declaration only).         */
/*                                                                         */
/*  Copyright 2000-2001 Catharon Productions Inc.                          */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* This defines the AH_GlyphLoader type in two different ways:           */
  /*                                                                       */
  /* - If the module is compiled within FreeType 2, the type is simply a   */
  /*   typedef to FT_GlyphLoader.                                          */
  /*                                                                       */
  /* - If the module is compiled as a standalone object, AH_GlyphLoader    */
  /*   has its own implementation.                                         */
  /*                                                                       */
  /*************************************************************************/


#ifndef __AHLOADER_H__
#define __AHLOADER_H__


#include <ft2build.h>


FT_BEGIN_HEADER


#ifdef _STANDALONE_

  typedef struct  AH_GlyphLoad_
  {
    FT_Outline    outline;       /* outline             */
    FT_UInt       num_subglyphs; /* number of subglyphs */
    FT_SubGlyph*  subglyphs;     /* subglyphs           */
    FT_Vector*    extra_points;  /* extra points table  */

  } AH_GlyphLoad;


  struct  AH_GlyphLoader_
  {
    FT_Memory     memory;
    FT_UInt       max_points;
    FT_UInt       max_contours;
    FT_UInt       max_subglyphs;
    FT_Bool       use_extra;

    AH_GlyphLoad  base;
    AH_GlyphLoad  current;

    void*         other;        /* for possible future extensions */
  };


  FT_LOCAL FT_Error
  AH_GlyphLoader_New( FT_Memory         memory,
                      AH_GlyphLoader**  aloader );

  FT_LOCAL FT_Error
  AH_GlyphLoader_Create_Extra( AH_GlyphLoader*  loader );

  FT_LOCAL void
  AH_GlyphLoader_Done( AH_GlyphLoader*  loader );

  FT_LOCAL void
  AH_GlyphLoader_Reset( AH_GlyphLoader*  loader );

  FT_LOCAL void
  AH_GlyphLoader_Rewind( AH_GlyphLoader*  loader );

  FT_LOCAL FT_Error
  AH_GlyphLoader_Check_Points( AH_GlyphLoader*  loader,
                               FT_UInt          n_points,
                               FT_UInt          n_contours );

  FT_LOCAL FT_Error
  AH_GlyphLoader_Check_Subglyphs( AH_GlyphLoader*  loader,
                                  FT_UInt          n_subs );

  FT_LOCAL void
  AH_GlyphLoader_Prepare( AH_GlyphLoader*  loader );

  FT_LOCAL void
  AH_GlyphLoader_Add( AH_GlyphLoader*  loader );

  FT_LOCAL FT_Error
  AH_GlyphLoader_Copy_Points( AH_GlyphLoader*  target,
                              FT_GlyphLoader*  source );

#else /* _STANDALONE */

#include FT_INTERNAL_OBJECTS_H

  #define AH_Load    FT_GlyphLoad
  #define AH_Loader  FT_GlyphLoader

  #define ah_loader_new              FT_GlyphLoader_New
  #define ah_loader_done             FT_GlyphLoader_Done
  #define ah_loader_reset            FT_GlyphLoader_Reset
  #define ah_loader_rewind           FT_GlyphLoader_Rewind
  #define ah_loader_create_extra     FT_GlyphLoader_Create_Extra
  #define ah_loader_check_points     FT_GlyphLoader_Check_Points
  #define ah_loader_check_subglyphs  FT_GlyphLoader_Check_Subglyphs
  #define ah_loader_prepare          FT_GlyphLoader_Prepare
  #define ah_loader_add              FT_GlyphLoader_Add
  #define ah_loader_copy_points      FT_GlyphLoader_Copy_Points

#endif /* _STANDALONE_ */


FT_END_HEADER

#endif /* __AHLOADER_H__ */


/* END */
