/***************************************************************************/
/*                                                                         */
/*  ahoptim.c                                                              */
/*                                                                         */
/*    FreeType auto hinting outline optimization (body).                   */
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
  /* This module is in charge of optimising the outlines produced by the   */
  /* auto-hinter in direct mode. This is required at small pixel sizes in  */
  /* order to ensure coherent spacing, among other things..                */
  /*                                                                       */
  /* The technique used in this module is a simplified simulated           */
  /* annealing.                                                            */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H        /* for ALLOC_ARRAY() and FREE() */
#include "ahoptim.h"


  /* define this macro to use brute force optimisation -- this is slow,  */
  /* but a good way to perfect the distortion function `by hand' through */
  /* tweaking                                                            */
#define AH_BRUTE_FORCE


#define xxxAH_DEBUG_OPTIM


#undef LOG
#ifdef AH_DEBUG_OPTIM

#define LOG( x )  optim_log ## x

#else

#define LOG( x )

#endif /* AH_DEBUG_OPTIM */


#ifdef AH_DEBUG_OPTIM

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define FLOAT( x )  ( (float)( (x) / 64.0 ) )

  static void
  optim_log( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );
  }


  static void
  AH_Dump_Stems( AH_Optimizer*  optimizer )
  {
    int       n;
    AH_Stem*  stem;


    stem = optimizer->stems;
    for ( n = 0; n < optimizer->num_stems; n++, stem++ )
    {
      LOG(( " %c%2d [%.1f:%.1f]={%.1f:%.1f}="
            "<%1.f..%1.f> force=%.1f speed=%.1f\n",
            optimizer->vertical ? 'V' : 'H', n,
            FLOAT( stem->edge1->opos ), FLOAT( stem->edge2->opos ),
            FLOAT( stem->edge1->pos ),  FLOAT( stem->edge2->pos ),
            FLOAT( stem->min_pos ),     FLOAT( stem->max_pos ),
            FLOAT( stem->force ),       FLOAT( stem->velocity ) ));
    }
  }


  static void
  AH_Dump_Stems2( AH_Optimizer*  optimizer )
  {
    int       n;
    AH_Stem*  stem;


    stem = optimizer->stems;
    for ( n = 0; n < optimizer->num_stems; n++, stem++ )
    {
      LOG(( " %c%2d [%.1f]=<%1.f..%1.f> force=%.1f speed=%.1f\n",
            optimizer->vertical ? 'V' : 'H', n,
            FLOAT( stem->pos ),
            FLOAT( stem->min_pos ), FLOAT( stem->max_pos ),
            FLOAT( stem->force ),   FLOAT( stem->velocity ) ));
    }
  }


  static void
  AH_Dump_Springs( AH_Optimizer*  optimizer )
  {
    int  n;
    AH_Spring*  spring;
    AH_Stem*    stems;


    spring = optimizer->springs;
    stems  = optimizer->stems;
    LOG(( "%cSprings ", optimizer->vertical ? 'V' : 'H' ));

    for ( n = 0; n < optimizer->num_springs; n++, spring++ )
    {
      LOG(( " [%d-%d:%.1f:%1.f:%.1f]",
            spring->stem1 - stems, spring->stem2 - stems,
            FLOAT( spring->owidth ),
            FLOAT( spring->stem2->pos -
                   ( spring->stem1->pos + spring->stem1->width ) ),
            FLOAT( spring->tension ) ));
    }

    LOG(( "\n" ));
  }

#endif /* AH_DEBUG_OPTIM */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   COMPUTE STEMS AND SPRINGS IN AN OUTLINE                       ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static int
  valid_stem_segments( AH_Segment*  seg1,
                       AH_Segment*  seg2 )
  {
    return seg1->serif == 0                   &&
           seg2                               &&
           seg2->link == seg1                 &&
           seg1->pos < seg2->pos              &&
           seg1->min_coord <= seg2->max_coord &&
           seg2->min_coord <= seg1->max_coord;
  }


  /* compute all stems in an outline */
  static int
  optim_compute_stems( AH_Optimizer*  optimizer )
  {
    AH_Outline*  outline = optimizer->outline;
    FT_Fixed     scale;
    FT_Memory    memory  = optimizer->memory;
    FT_Error     error   = 0;
    FT_Int       dimension;
    AH_Edge*     edges;
    AH_Edge*     edge_limit;
    AH_Stem**    p_stems;
    FT_Int*      p_num_stems;


    edges      = outline->horz_edges;
    edge_limit = edges + outline->num_hedges;
    scale      = outline->y_scale;

    p_stems     = &optimizer->horz_stems;
    p_num_stems = &optimizer->num_hstems;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Stem*  stems     = 0;
      FT_Int    num_stems = 0;
      AH_Edge*  edge;


      /* first of all, count the number of stems in this direction */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AH_Segment*  seg = edge->first;


        do
        {
          if (valid_stem_segments( seg, seg->link ) )
            num_stems++;

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }

      /* now allocate the stems and build their table */
      if ( num_stems > 0 )
      {
        AH_Stem*  stem;


        if ( ALLOC_ARRAY( stems, num_stems, AH_Stem ) )
          goto Exit;

        stem = stems;
        for ( edge = edges; edge < edge_limit; edge++ )
        {
          AH_Segment*  seg = edge->first;
          AH_Segment*  seg2;


          do
          {
            seg2 = seg->link;
            if ( valid_stem_segments( seg, seg2 ) )
            {
              AH_Edge*  edge1 = seg->edge;
              AH_Edge*  edge2 = seg2->edge;


              stem->edge1  = edge1;
              stem->edge2  = edge2;
              stem->opos   = edge1->opos;
              stem->pos    = edge1->pos;
              stem->owidth = edge2->opos - edge1->opos;
              stem->width  = edge2->pos  - edge1->pos;

              /* compute min_coord and max_coord */
              {
                FT_Pos  min_coord = seg->min_coord;
                FT_Pos  max_coord = seg->max_coord;


                if ( seg2->min_coord > min_coord )
                  min_coord = seg2->min_coord;

                if ( seg2->max_coord < max_coord )
                  max_coord = seg2->max_coord;

                stem->min_coord = min_coord;
                stem->max_coord = max_coord;
              }

              /* compute minimum and maximum positions for stem --   */
              /* note that the left-most/bottom-most stem has always */
              /* a fixed position                                    */
              if ( stem == stems || edge1->blue_edge || edge2->blue_edge )
              {
                /* this stem cannot move; it is snapped to a blue edge */
                stem->min_pos = stem->pos;
                stem->max_pos = stem->pos;
              }
              else
              {
                /* this edge can move; compute its min and max positions */
                FT_Pos  pos1 = stem->opos;
                FT_Pos  pos2 = pos1 + stem->owidth - stem->width;
                FT_Pos  min1 = pos1 & -64;
                FT_Pos  min2 = pos2 & -64;


                stem->min_pos = min1;
                stem->max_pos = min1 + 64;
                if ( min2 < min1 )
                  stem->min_pos = min2;
                else
                  stem->max_pos = min2 + 64;

                /* XXX: just to see what it does */
                stem->max_pos += 64;

                /* just for the case where direct hinting did some */
                /* incredible things (e.g. blue edge shifts)       */
                if ( stem->min_pos > stem->pos )
                  stem->min_pos = stem->pos;

                if ( stem->max_pos < stem->pos )
                  stem->max_pos = stem->pos;
              }

              stem->velocity = 0;
              stem->force    = 0;

              stem++;
            }
            seg = seg->edge_next;

          } while ( seg != edge->first );
        }
      }

      *p_stems     = stems;
      *p_num_stems = num_stems;

      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
      scale      = outline->x_scale;

      p_stems     = &optimizer->vert_stems;
      p_num_stems = &optimizer->num_vstems;
    }

  Exit:

#ifdef AH_DEBUG_OPTIM
    AH_Dump_Stems( optimizer );
#endif

    return error;
  }


  /* returns the spring area between two stems, 0 if none */
  static FT_Pos
  stem_spring_area( AH_Stem*  stem1,
                    AH_Stem*  stem2 )
  {
    FT_Pos  area1 = stem1->max_coord - stem1->min_coord;
    FT_Pos  area2 = stem2->max_coord - stem2->min_coord;
    FT_Pos  min   = stem1->min_coord;
    FT_Pos  max   = stem1->max_coord;
    FT_Pos  area;


    /* order stems */
    if ( stem2->opos <= stem1->opos + stem1->owidth )
      return 0;

    if ( min < stem2->min_coord )
      min = stem2->min_coord;

    if ( max < stem2->max_coord )
      max = stem2->max_coord;

    area = ( max-min );
    if ( 2 * area < area1 && 2 * area < area2 )
      area = 0;

    return area;
  }


  /* compute all springs in an outline */
  static int
  optim_compute_springs( AH_Optimizer*  optimizer )
  {
    /* basically, a spring exists between two stems if most of their */
    /* surface is aligned                                            */
    FT_Memory    memory  = optimizer->memory;

    AH_Stem*     stems;
    AH_Stem*     stem_limit;
    AH_Stem*     stem;
    int          dimension;
    int          error = 0;

    FT_Int*      p_num_springs;
    AH_Spring**  p_springs;


    stems      = optimizer->horz_stems;
    stem_limit = stems + optimizer->num_hstems;

    p_springs     = &optimizer->horz_springs;
    p_num_springs = &optimizer->num_hsprings;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      FT_Int      num_springs = 0;
      AH_Spring*  springs     = 0;


      /* first of all, count stem springs */
      for ( stem = stems; stem + 1 < stem_limit; stem++ )
      {
        AH_Stem*  stem2;


        for ( stem2 = stem+1; stem2 < stem_limit; stem2++ )
          if ( stem_spring_area( stem, stem2 ) )
            num_springs++;
      }

      /* then allocate and build the springs table */
      if ( num_springs > 0 )
      {
        AH_Spring*  spring;


        /* allocate table of springs */
        if ( ALLOC_ARRAY( springs, num_springs, AH_Spring ) )
          goto Exit;

        /* fill the springs table */
        spring = springs;
        for ( stem = stems; stem+1 < stem_limit; stem++ )
        {
          AH_Stem*  stem2;
          FT_Pos    area;


          for ( stem2 = stem + 1; stem2 < stem_limit; stem2++ )
          {
            area = stem_spring_area( stem, stem2 );
            if ( area )
            {
              /* add a new spring here */
              spring->stem1   = stem;
              spring->stem2   = stem2;
              spring->owidth  = stem2->opos - ( stem->opos + stem->owidth );
              spring->tension = 0;

              spring++;
            }
          }
        }
      }
      *p_num_springs = num_springs;
      *p_springs     = springs;

      stems      = optimizer->vert_stems;
      stem_limit = stems + optimizer->num_vstems;

      p_springs     = &optimizer->vert_springs;
      p_num_springs = &optimizer->num_vsprings;
    }

  Exit:

#ifdef AH_DEBUG_OPTIM
    AH_Dump_Springs( optimizer );
#endif

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   OPTIMIZE THROUGH MY STRANGE SIMULATED ANNEALING ALGO ;-)      ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#ifndef AH_BRUTE_FORCE

  /* compute all spring tensions */
  static void
  optim_compute_tensions( AH_Optimizer*  optimizer )
  {
    AH_Spring*  spring = optimizer->springs;
    AH_Spring*  limit  = spring + optimizer->num_springs;


    for ( ; spring < limit; spring++ )
    {
      AH_Stem*  stem1 = spring->stem1;
      AH_Stem*  stem2 = spring->stem2;
      FT_Int    status;

      FT_Pos  width;
      FT_Pos  tension;
      FT_Pos  sign;


      /* compute the tension; it simply is -K*(new_width-old_width) */
      width   = stem2->pos - ( stem1->pos + stem1->width );
      tension = width - spring->owidth;

      sign = 1;
      if ( tension < 0 )
      {
        sign    = -1;
        tension = -tension;
      }

      if ( width <= 0 )
        tension = 32000;
      else
        tension = ( tension << 10 ) / width;

      tension = -sign * FT_MulFix( tension, optimizer->tension_scale );
      spring->tension = tension;

      /* now, distribute tension among the englobing stems, if they */
      /* are able to move                                           */
      status = 0;
      if ( stem1->pos <= stem1->min_pos )
        status |= 1;
      if ( stem2->pos >= stem2->max_pos )
        status |= 2;

      if ( !status )
        tension /= 2;

      if ( ( status & 1 ) == 0 )
        stem1->force -= tension;

      if ( ( status & 2 ) == 0 )
        stem2->force += tension;
    }
  }


  /* compute all stem movements -- returns 0 if nothing moved */
  static int
  optim_compute_stem_movements( AH_Optimizer*  optimizer )
  {
    AH_Stem*  stems = optimizer->stems;
    AH_Stem*  limit = stems + optimizer->num_stems;
    AH_Stem*  stem  = stems;
    int       moved = 0;


    /* set initial forces to velocity */
    for ( stem = stems; stem < limit; stem++ )
    {
      stem->force     = stem->velocity;
      stem->velocity /= 2;                  /* XXX: Heuristics */
    }

    /* compute the sum of forces applied on each stem */
    optim_compute_tensions( optimizer );

#ifdef AH_DEBUG_OPTIM
    AH_Dump_Springs( optimizer );
    AH_Dump_Stems2( optimizer );
#endif

    /* now, see whether something can move */
    for ( stem = stems; stem < limit; stem++ )
    {
      if ( stem->force > optimizer->tension_threshold )
      {
        /* there is enough tension to move the stem to the right */
        if ( stem->pos < stem->max_pos )
        {
          stem->pos     += 64;
          stem->velocity = stem->force / 2;
          moved          = 1;
        }
        else
          stem->velocity = 0;
      }
      else if ( stem->force < optimizer->tension_threshold )
      {
        /* there is enough tension to move the stem to the left */
        if ( stem->pos > stem->min_pos )
        {
          stem->pos     -= 64;
          stem->velocity = stem->force / 2;
          moved          = 1;
        }
        else
          stem->velocity = 0;
      }
    }

    /* return 0 if nothing moved */
    return moved;
  }

#endif /* AH_BRUTE_FORCE */


  /* compute current global distortion from springs */
  static FT_Pos
  optim_compute_distortion( AH_Optimizer*  optimizer )
  {
    AH_Spring*  spring = optimizer->springs;
    AH_Spring*  limit  = spring + optimizer->num_springs;
    FT_Pos      distortion = 0;


    for ( ; spring < limit; spring++ )
    {
      AH_Stem*  stem1 = spring->stem1;
      AH_Stem*  stem2 = spring->stem2;
      FT_Pos  width;

      width  = stem2->pos - ( stem1->pos + stem1->width );
      width -= spring->owidth;
      if ( width < 0 )
        width = -width;

      distortion += width;
    }

    return distortion;
  }


  /* record stems configuration in `best of' history */
  static void
  optim_record_configuration( AH_Optimizer*  optimizer )
  {
    FT_Pos             distortion;
    AH_Configuration*  configs = optimizer->configs;
    AH_Configuration*  limit   = configs + optimizer->num_configs;
    AH_Configuration*  config;


    distortion = optim_compute_distortion( optimizer );
    LOG(( "config distortion = %.1f ", FLOAT( distortion * 64 ) ));

    /* check that we really need to add this configuration to our */
    /* sorted history                                             */
    if ( limit > configs && limit[-1].distortion < distortion )
    {
      LOG(( "ejected\n" ));
      return;
    }

    /* add new configuration at the end of the table */
    {
      int  n;


      config = limit;
      if ( optimizer->num_configs < AH_MAX_CONFIGS )
        optimizer->num_configs++;
      else
        config--;

      config->distortion = distortion;

      for ( n = 0; n < optimizer->num_stems; n++ )
        config->positions[n] = optimizer->stems[n].pos;
    }

    /* move the current configuration towards the front of the list */
    /* when necessary -- yes this is slow bubble sort ;-)           */
    while ( config > configs && config[0].distortion < config[-1].distortion )
    {
      AH_Configuration  temp;


      config--;
      temp      = config[0];
      config[0] = config[1];
      config[1] = temp;
    }
    LOG(( "recorded!\n" ));
  }


#ifdef AH_BRUTE_FORCE

  /* optimize outline in a single direction */
  static void
  optim_compute( AH_Optimizer*  optimizer )
  {
    int       n;
    FT_Bool   moved;

    AH_Stem*  stem  = optimizer->stems;
    AH_Stem*  limit = stem + optimizer->num_stems;


    /* empty, exit */
    if ( stem >= limit )
      return;

    optimizer->num_configs = 0;

    stem = optimizer->stems;
    for ( ; stem < limit; stem++ )
      stem->pos = stem->min_pos;

    do
    {
      /* record current configuration */
      optim_record_configuration( optimizer );

      /* now change configuration */
      moved = 0;
      for ( stem = optimizer->stems; stem < limit; stem++ )
      {
        if ( stem->pos < stem->max_pos )
        {
          stem->pos += 64;
          moved      = 1;
          break;
        }

        stem->pos = stem->min_pos;
      }
    } while ( moved );

    /* now, set the best stem positions */
    for ( n = 0; n < optimizer->num_stems; n++ )
    {
      AH_Stem*  stem = optimizer->stems + n;
      FT_Pos    pos  = optimizer->configs[0].positions[n];


      stem->edge1->pos = pos;
      stem->edge2->pos = pos + stem->width;

      stem->edge1->flags |= ah_edge_done;
      stem->edge2->flags |= ah_edge_done;
    }
  }

#else /* AH_BRUTE_FORCE */

  /* optimize outline in a single direction */
  static void
  optim_compute( AH_Optimizer*  optimizer )
  {
    int  n, counter, counter2;


    optimizer->num_configs       = 0;
    optimizer->tension_scale     = 0x80000L;
    optimizer->tension_threshold = 64;

    /* record initial configuration threshold */
    optim_record_configuration( optimizer );

    counter = 0;
    for ( counter2 = optimizer->num_stems*8; counter2 >= 0; counter2-- )
    {
      if ( counter == 0 )
        counter = 2 * optimizer->num_stems;

      if ( !optim_compute_stem_movements( optimizer ) )
        break;

      optim_record_configuration( optimizer );

      counter--;
      if ( counter == 0 )
        optimizer->tension_scale /= 2;
    }

    /* now, set the best stem positions */
    for ( n = 0; n < optimizer->num_stems; n++ )
    {
      AH_Stem*  stem = optimizer->stems + n;
      FT_Pos    pos  = optimizer->configs[0].positions[n];


      stem->edge1->pos = pos;
      stem->edge2->pos = pos + stem->width;

      stem->edge1->flags |= ah_edge_done;
      stem->edge2->flags |= ah_edge_done;
    }
  }

#endif /* AH_BRUTE_FORCE */


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   HIGH-LEVEL OPTIMIZER API                                      ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* releases the optimization data */
  void
  AH_Optimizer_Done( AH_Optimizer*  optimizer )
  {
    if ( optimizer )
    {
      FT_Memory  memory = optimizer->memory;


      FREE( optimizer->horz_stems );
      FREE( optimizer->vert_stems );
      FREE( optimizer->horz_springs );
      FREE( optimizer->vert_springs );
      FREE( optimizer->positions );
    }
  }


  /* loads the outline into the optimizer */
  int
  AH_Optimizer_Init( AH_Optimizer*  optimizer,
                     AH_Outline*    outline,
                     FT_Memory      memory )
  {
    FT_Error  error;


    MEM_Set( optimizer, 0, sizeof ( *optimizer ) );
    optimizer->outline = outline;
    optimizer->memory  = memory;

    LOG(( "initializing new optimizer\n" ));
    /* compute stems and springs */
    error = optim_compute_stems  ( optimizer ) ||
            optim_compute_springs( optimizer );
    if ( error )
      goto Fail;

    /* allocate stem positions history and configurations */
    {
      int  n, max_stems;


      max_stems = optimizer->num_hstems;
      if ( max_stems < optimizer->num_vstems )
        max_stems = optimizer->num_vstems;

      if ( ALLOC_ARRAY( optimizer->positions,
                        max_stems * AH_MAX_CONFIGS, FT_Pos ) )
        goto Fail;

      optimizer->num_configs = 0;
      for ( n = 0; n < AH_MAX_CONFIGS; n++ )
        optimizer->configs[n].positions = optimizer->positions +
                                          n * max_stems;
    }

    return error;

  Fail:
    AH_Optimizer_Done( optimizer );
    return error;
  }


  /* compute optimal outline */
  void
  AH_Optimizer_Compute( AH_Optimizer*  optimizer )
  {
    optimizer->num_stems   = optimizer->num_hstems;
    optimizer->stems       = optimizer->horz_stems;
    optimizer->num_springs = optimizer->num_hsprings;
    optimizer->springs     = optimizer->horz_springs;

    if ( optimizer->num_springs > 0 )
    {
      LOG(( "horizontal optimization ------------------------\n" ));
      optim_compute( optimizer );
    }

    optimizer->num_stems   = optimizer->num_vstems;
    optimizer->stems       = optimizer->vert_stems;
    optimizer->num_springs = optimizer->num_vsprings;
    optimizer->springs     = optimizer->vert_springs;

    if ( optimizer->num_springs )
    {
      LOG(( "vertical optimization --------------------------\n" ));
      optim_compute( optimizer );
    }
  }


/* END */
