/***************************************************************************/
/*                                                                         */
/*  ahoptim.h                                                              */
/*                                                                         */
/*    FreeType auto hinting outline optimization (declaration).            */
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


#ifndef __AHOPTIM_H__
#define __AHOPTIM_H__


#include <ft2build.h>
#include "ahtypes.h"


FT_BEGIN_HEADER


  /* the maximal number of stem configurations to record */
  /* during optimization                                 */
#define AH_MAX_CONFIGS  8


  typedef struct  AH_Stem_
  {
    FT_Pos    pos;       /* current position        */
    FT_Pos    velocity;  /* current velocity        */
    FT_Pos    force;     /* sum of current forces   */
    FT_Pos    width;     /* normalized width        */

    FT_Pos    min_pos;   /* minimum grid position */
    FT_Pos    max_pos;   /* maximum grid position */

    AH_Edge*  edge1;     /* left/bottom edge */
    AH_Edge*  edge2;     /* right/top edge   */

    FT_Pos    opos;      /* original position */
    FT_Pos    owidth;    /* original width    */

    FT_Pos    min_coord; /* minimum coordinate */
    FT_Pos    max_coord; /* maximum coordinate */

  } AH_Stem;


  /* A spring between two stems */
  typedef struct  AH_Spring_
  {
    AH_Stem*  stem1;
    AH_Stem*  stem2;
    FT_Pos    owidth;   /* original width  */
    FT_Pos    tension;  /* current tension */

  } AH_Spring;


  /* A configuration records the position of each stem at a given time  */
  /* as well as the associated distortion                               */
  typedef struct AH_Configuration_
  {
    FT_Pos*  positions;
    FT_Long  distortion;

  } AH_Configuration;


  typedef struct  AH_Optimizer_
  {
    FT_Memory         memory;
    AH_Outline*       outline;

    FT_Int            num_hstems;
    AH_Stem*          horz_stems;

    FT_Int            num_vstems;
    AH_Stem*          vert_stems;

    FT_Int            num_hsprings;
    FT_Int            num_vsprings;
    AH_Spring*        horz_springs;
    AH_Spring*        vert_springs;

    FT_Int            num_configs;
    AH_Configuration  configs[AH_MAX_CONFIGS];
    FT_Pos*           positions;

    /* during each pass, use these instead */
    FT_Int            num_stems;
    AH_Stem*          stems;

    FT_Int            num_springs;
    AH_Spring*        springs;
    FT_Bool           vertical;

    FT_Fixed          tension_scale;
    FT_Pos            tension_threshold;

  } AH_Optimizer;


  /* loads the outline into the optimizer */
  int
  AH_Optimizer_Init( AH_Optimizer*  optimizer,
                     AH_Outline*    outline,
                     FT_Memory      memory );


  /* compute optimal outline */
  void
  AH_Optimizer_Compute( AH_Optimizer*  optimizer );


  /* release the optimization data */
  void
  AH_Optimizer_Done( AH_Optimizer*  optimizer );


FT_END_HEADER

#endif /* __AHOPTIM_H__ */


/* END */
