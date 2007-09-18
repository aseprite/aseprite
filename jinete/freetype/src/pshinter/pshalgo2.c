/***************************************************************************/
/*                                                                         */
/*  pshalgo2.c                                                             */
/*                                                                         */
/*    PostScript hinting algorithm 2 (body).                               */
/*                                                                         */
/*  Copyright 2001 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include "pshalgo2.h"

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshalgo2

#ifdef DEBUG_HINTER
  extern PSH2_Hint_Table  ps2_debug_hint_table = 0;
  extern PSH2_HintFunc    ps2_debug_hint_func  = 0;
  extern PSH2_Glyph       ps2_debug_glyph      = 0;
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  BASIC HINTS RECORDINGS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* return true iff two stem hints overlap */
  static FT_Int
  psh2_hint_overlap( PSH2_Hint  hint1,
                     PSH2_Hint  hint2 )
  {
    return ( hint1->org_pos + hint1->org_len >= hint2->org_pos &&
             hint2->org_pos + hint2->org_len >= hint1->org_pos );
  }


  /* destroy hints table */
  static void
  psh2_hint_table_done( PSH2_Hint_Table  table,
                        FT_Memory        memory )
  {
    FREE( table->zones );
    table->num_zones = 0;
    table->zone      = 0;

    FREE( table->sort );
    FREE( table->hints );
    table->num_hints   = 0;
    table->max_hints   = 0;
    table->sort_global = 0;
  }


  /* deactivate all hints in a table */
  static void
  psh2_hint_table_deactivate( PSH2_Hint_Table  table )
  {
    FT_UInt   count = table->max_hints;
    PSH2_Hint  hint  = table->hints;


    for ( ; count > 0; count--, hint++ )
    {
      psh2_hint_deactivate( hint );
      hint->order = -1;
    }
  }


  /* internal function used to record a new hint */
  static void
  psh2_hint_table_record( PSH2_Hint_Table  table,
                          FT_UInt          index )
  {
    PSH2_Hint  hint = table->hints + index;


    if ( index >= table->max_hints )
    {
      FT_ERROR(( "%s.activate: invalid hint index %d\n", index ));
      return;
    }

    /* ignore active hints */
    if ( psh2_hint_is_active( hint ) )
      return;

    psh2_hint_activate( hint );

    /* now scan the current active hint set in order to determine */
    /* if we are overlapping with another segment                 */
    {
      PSH2_Hint*  sorted = table->sort_global;
      FT_UInt     count  = table->num_hints;
      PSH2_Hint   hint2;


      hint->parent = 0;
      for ( ; count > 0; count--, sorted++ )
      {
        hint2 = sorted[0];

        if ( psh2_hint_overlap( hint, hint2 ) )
        {
          hint->parent = hint2;
          break;
        }
      }
    }

    if ( table->num_hints < table->max_hints )
      table->sort_global[table->num_hints++] = hint;
    else
      FT_ERROR(( "%s.activate: too many sorted hints!  BUG!\n",
                 "ps.fitter" ));
  }


  static void
  psh2_hint_table_record_mask( PSH2_Hint_Table  table,
                               PS_Mask          hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   index, limit;


    limit = hint_mask->num_bits;

    for ( index = 0; index < limit; index++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
        psh2_hint_table_record( table, index );

      mask >>= 1;
    }
  }


  /* create hints table */
  static FT_Error
  psh2_hint_table_init( PSH2_Hint_Table  table,
                        PS_Hint_Table    hints,
                        PS_Mask_Table    hint_masks,
                        PS_Mask_Table    counter_masks,
                        FT_Memory        memory )
  {
    FT_UInt   count = hints->num_hints;
    FT_Error  error;

    FT_UNUSED( counter_masks );


    /* allocate our tables */
    if ( ALLOC_ARRAY( table->sort,  2 * count,     PSH2_Hint    ) ||
         ALLOC_ARRAY( table->hints,     count,     PSH2_HintRec ) ||
         ALLOC_ARRAY( table->zones, 2 * count + 1, PSH2_ZoneRec ) )
      goto Exit;

    table->max_hints   = count;
    table->sort_global = table->sort + count;
    table->num_hints   = 0;
    table->num_zones   = 0;
    table->zone        = 0;

    /* now, initialize the "hints" array */
    {
      PSH2_Hint  write = table->hints;
      PS_Hint    read  = hints->hints;


      for ( ; count > 0; count--, write++, read++ )
      {
        write->org_pos = read->pos;
        write->org_len = read->len;
        write->flags   = read->flags;
      }
    }

    /* we now need to determine the initial "parent" stems; first  */
    /* activate the hints that are given by the initial hint masks */
    if ( hint_masks )
    {
      FT_UInt  Count = hint_masks->num_masks;
      PS_Mask  Mask  = hint_masks->masks;


      table->hint_masks = hint_masks;

      for ( ; Count > 0; Count--, Mask++ )
        psh2_hint_table_record_mask( table, Mask );
    }

    /* now, do a linear parse in case some hints were left alone */
    if ( table->num_hints != table->max_hints )
    {
      FT_UInt   Index, Count;


      FT_ERROR(( "%s.init: missing/incorrect hint masks!\n" ));
      Count = table->max_hints;
      for ( Index = 0; Index < Count; Index++ )
        psh2_hint_table_record( table, Index );
    }

  Exit:
    return error;
  }


  static void
  psh2_hint_table_activate_mask( PSH2_Hint_Table  table,
                                 PS_Mask          hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   index, limit, count;


    limit = hint_mask->num_bits;
    count = 0;

    psh2_hint_table_deactivate( table );

    for ( index = 0; index < limit; index++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
      {
        PSH2_Hint  hint = &table->hints[index];


        if ( !psh2_hint_is_active( hint ) )
        {
          FT_UInt     count2;

#if 0
          PSH2_Hint*  sort = table->sort;
          PSH2_Hint   hint2;

          for ( count2 = count; count2 > 0; count2--, sort++ )
          {
            hint2 = sort[0];
            if ( psh2_hint_overlap( hint, hint2 ) )
              FT_ERROR(( "%s.activate_mask: found overlapping hints\n",
                         "psf.hint" ));
          }
#else
          count2 = 0;
#endif

          if ( count2 == 0 )
          {
            psh2_hint_activate( hint );
            if ( count < table->max_hints )
              table->sort[count++] = hint;
            else
              FT_ERROR(( "%s.activate_mask: too many active hints\n",
                         "psf.hint" ));
          }
        }
      }

      mask >>= 1;
    }
    table->num_hints = count;

    /* now, sort the hints; they are guaranteed to not overlap */
    /* so we can compare their "org_pos" field directly        */
    {
      FT_Int      i1, i2;
      PSH2_Hint   hint1, hint2;
      PSH2_Hint*  sort = table->sort;


      /* a simple bubble sort will do, since in 99% of cases, the hints */
      /* will be already sorted -- and the sort will be linear          */
      for ( i1 = 1; i1 < (FT_Int)count; i1++ )
      {
        hint1 = sort[i1];
        for ( i2 = i1 - 1; i2 >= 0; i2-- )
        {
          hint2 = sort[i2];

          if ( hint2->org_pos < hint1->org_pos )
            break;

          sort[i2 + 1] = hint2;
          sort[i2]     = hint1;
        }
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               HINTS GRID-FITTING AND OPTIMIZATION             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#ifdef DEBUG_HINTER
  static void
  ps2_simple_scale( PSH2_Hint_Table  table,
                    FT_Fixed         scale,
                    FT_Fixed         delta,
                    FT_Int           dimension )
  {
    PSH2_Hint  hint;
    FT_UInt    count;


    for ( count = 0; count < table->max_hints; count++ )
    {
      hint = table->hints + count;

      hint->cur_pos = FT_MulFix( hint->org_pos, scale ) + delta;
      hint->cur_len = FT_MulFix( hint->org_len, scale );

      if ( ps2_debug_hint_func )
        ps2_debug_hint_func( hint, dimension );
    }
  }
#endif


  static void
  psh2_hint_align( PSH2_Hint    hint,
                   PSH_Globals  globals,
                   FT_Int       dimension )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh2_hint_is_fitted(hint) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Pos  fit_center;
      FT_Pos  fit_len;

      PSH_AlignmentRec  align;


      /* compute fitted width/height */
      fit_len = 0;
      if ( hint->org_len )
      {
        fit_len = psh_dimension_snap_width( dim, hint->org_len );
        if ( fit_len < 64 )
          fit_len = 64;
        else
          fit_len = ( fit_len + 32 ) & -64;
      }

      hint->cur_len = fit_len;

      /* check blue zones for horizontal stems */
      align.align = 0;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH2_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh2_hint_is_fitted( parent ) )
              psh2_hint_align( parent, globals, dimension );

            par_org_center = parent->org_pos + ( parent->org_len / 2);
            par_cur_center = parent->cur_pos + ( parent->cur_len / 2);
            cur_org_center = hint->org_pos   + ( hint->org_len   / 2);

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
#if 0
            if ( cur_delta >= 0 )
              cur_delta = ( cur_delta + 16 ) & -64;
            else
              cur_delta = -( (-cur_delta + 16 ) & -64 );
#endif
            pos = par_cur_center + cur_delta - ( len >> 1 );
          }

          /* normal processing */
          if ( ( fit_len / 64 ) & 1 )
          {
            /* odd number of pixels */
            fit_center = ( ( pos + ( len >> 1 ) ) & -64 ) + 32;
          }
          else
          {
            /* even number of pixels */
            fit_center = ( pos + ( len >> 1 ) + 32 ) & -64;
          }

          hint->cur_pos = fit_center - ( fit_len >> 1 );
        }
      }

      psh2_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps2_debug_hint_func )
        ps2_debug_hint_func( hint, dimension );
#endif
    }
  }


  static void
  psh2_hint_table_align_hints( PSH2_Hint_Table  table,
                               PSH_Globals      globals,
                               FT_Int           dimension )
  {
    PSH2_Hint      hint;
    FT_UInt        count;

#ifdef DEBUG_HINTER
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( ps_debug_no_vert_hints && dimension == 0 )
    {
      ps2_simple_scale( table, scale, delta, dimension );
      return;
    }

    if ( ps_debug_no_horz_hints && dimension == 1 )
    {
      ps2_simple_scale( table, scale, delta, dimension );
      return;
    }
#endif

    hint  = table->hints;
    count = table->max_hints;

    for ( ; count > 0; count--, hint++ )
      psh2_hint_align( hint, globals, dimension );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                POINTS INTERPOLATION ROUTINES                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define PSH2_ZONE_MIN  -3200000
#define PSH2_ZONE_MAX  +3200000

#define xxDEBUG_ZONES


#ifdef DEBUG_ZONES

#include <stdio.h>

  static void
  print_zone( PSH2_Zone  zone )
  {
    printf( "zone [scale,delta,min,max] = [%.3f,%.3f,%d,%d]\n",
             zone->scale/65536.0,
             zone->delta/64.0,
             zone->min,
             zone->max );
  }

#else

#define print_zone( x )   do { } while ( 0 )

#endif

#if 0
  /* setup interpolation zones once the hints have been grid-fitted */
  /* by the optimizer                                               */
  static void
  psh2_hint_table_setup_zones( PSH2_Hint_Table  table,
                               FT_Fixed         scale,
                               FT_Fixed         delta )
  {
    FT_UInt     count;
    PSH2_Zone   zone;
    PSH2_Hint  *sort, hint, hint2;


    zone = table->zones;

    /* special case, no hints defined */
    if ( table->num_hints == 0 )
    {
      zone->scale = scale;
      zone->delta = delta;
      zone->min   = PSH2_ZONE_MIN;
      zone->max   = PSH2_ZONE_MAX;

      table->num_zones = 1;
      table->zone      = zone;
      return;
    }

    /* the first zone is before the first hint */
    /* x' = (x-x0)*s + x0' = x*s + ( x0' - x0*s ) */
    sort = table->sort;
    hint = sort[0];

    zone->scale = scale;
    zone->delta = hint->cur_pos - FT_MulFix( hint->org_pos, scale );
    zone->min   = PSH2_ZONE_MIN;
    zone->max   = hint->org_pos;

    print_zone( zone );

    zone++;

    for ( count = table->num_hints; count > 0; count-- )
    {
      FT_Fixed  scale2;


      if ( hint->org_len > 0 )
      {
        /* setup a zone for inner-stem interpolation */
        /* (x' - x0') = (x - x0)*(x1'-x0')/(x1-x0)   */
        /* x' = x*s2 + x0' - x0*s2                   */

        scale2      = FT_DivFix( hint->cur_len, hint->org_len );
        zone->scale = scale2;
        zone->min   = hint->org_pos;
        zone->max   = hint->org_pos + hint->org_len;
        zone->delta = hint->cur_pos - FT_MulFix( zone->min, scale2 );

        print_zone( zone );

        zone++;
      }

      if ( count == 1 )
        break;

      sort++;
      hint2 = sort[0];

      /* setup zone for inter-stem interpolation */
      /* (x'-x1') = (x-x1)*(x2'-x1')/(x2-x1)     */
      /* x' = x*s3 + x1' - x1*s3                 */

      scale2 = FT_DivFix( hint2->cur_pos - (hint->cur_pos + hint->cur_len),
                          hint2->org_pos - (hint->org_pos + hint->org_len) );
      zone->scale = scale2;
      zone->min   = hint->org_pos + hint->org_len;
      zone->max   = hint2->org_pos;
      zone->delta = hint->cur_pos + hint->cur_len -
                    FT_MulFix( zone->min, scale2 );

      print_zone( zone );

      zone++;

      hint = hint2;
    }

    /* the last zone */
    zone->scale = scale;
    zone->min   = hint->org_pos + hint->org_len;
    zone->max   = PSH2_ZONE_MAX;
    zone->delta = hint->cur_pos + hint->cur_len -
                  FT_MulFix( zone->min, scale );

    print_zone( zone );

    zone++;

    table->num_zones = zone - table->zones;
    table->zone      = table->zones;
  }
#endif

#if 0
  /* tune a single coordinate with the current interpolation zones */
  static FT_Pos
  psh2_hint_table_tune_coord( PSH2_Hint_Table  table,
                              FT_Int           coord )
  {
    PSH2_Zone   zone;


    zone = table->zone;

    if ( coord < zone->min )
    {
      do
      {
        if ( zone == table->zones )
          break;

        zone--;

      } while ( coord < zone->min );
      table->zone = zone;
    }
    else if ( coord > zone->max )
    {
      do
      {
        if ( zone == table->zones + table->num_zones - 1 )
          break;

        zone++;

      } while ( coord > zone->max );
      table->zone = zone;
    }

    return FT_MulFix( coord, zone->scale ) + zone->delta;
  }
#endif

#if 0
 /* tune a given outline with current interpolation zones */
 /* the function only works in a single dimension..       */
  static void
  psh2_hint_table_tune_outline( PSH2_Hint_Table  table,
                                FT_Outline*      outline,
                                PSH_Globals      globals,
                                FT_Int           dimension )

  {
    FT_UInt        count, first, last;
    PS_Mask_Table  hint_masks = table->hint_masks;
    PS_Mask        mask;
    PSH_Dimension  dim        = &globals->dimension[dimension];
    FT_Fixed       scale      = dim->scale_mult;
    FT_Fixed       delta      = dim->scale_delta;


    if ( hint_masks && hint_masks->num_masks > 0 )
    {
      first = 0;
      mask  = hint_masks->masks;
      count = hint_masks->num_masks;

      for ( ; count > 0; count--, mask++ )
      {
        last = mask->end_point;

        if ( last > first )
        {
          FT_Vector*   vec;
          FT_Int       count2;


          psh2_hint_table_activate_mask( table, mask );
          psh2_hint_table_optimize( table, globals, outline, dimension );
          psh2_hint_table_setup_zones( table, scale, delta );
          last = mask->end_point;

          vec    = outline->points + first;
          count2 = last - first;

          for ( ; count2 > 0; count2--, vec++ )
          {
            FT_Pos  x, *px;


            px  = dimension ? &vec->y : &vec->x;
            x   = *px;

            *px = psh2_hint_table_tune_coord( table, (FT_Int)x );
          }
        }

        first = last;
      }
    }
    else    /* no hints in this glyph, simply scale the outline */
    {
      FT_Vector*  vec;


      vec   = outline->points;
      count = outline->n_points;

      if ( dimension == 0 )
      {
        for ( ; count > 0; count--, vec++ )
          vec->x = FT_MulFix( vec->x, scale ) + delta;
      }
      else
      {
        for ( ; count > 0; count--, vec++ )
          vec->y = FT_MulFix( vec->y, scale ) + delta;
      }
    }
  }
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    HINTER GLYPH MANAGEMENT                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static int
  psh2_point_is_extremum( PSH2_Point  point )
  {
    PSH2_Point  before = point;
    PSH2_Point  after  = point;
    FT_Pos      d_before;
    FT_Pos      d_after;


    do
    {
      before = before->prev;
      if ( before == point )
        return 0;

      d_before = before->org_u - point->org_u;

    } while ( d_before == 0 );

    do
    {
      after = after->next;
      if ( after == point )
        return 0;

      d_after = after->org_u - point->org_u;

    } while ( d_after == 0 );

    return ( ( d_before > 0 && d_after > 0 ) ||
             ( d_before < 0 && d_after < 0 ) );
  }


  static void
  psh2_glyph_done( PSH2_Glyph  glyph )
  {
    FT_Memory  memory = glyph->memory;


    psh2_hint_table_done( &glyph->hint_tables[1], memory );
    psh2_hint_table_done( &glyph->hint_tables[0], memory );

    FREE( glyph->points );
    FREE( glyph->contours );

    glyph->num_points   = 0;
    glyph->num_contours = 0;

    glyph->memory = 0;
  }


  static int
  psh2_compute_dir( FT_Pos  dx,
                    FT_Pos  dy )
  {
    FT_Pos  ax, ay;
    int     result = PSH2_DIR_NONE;


    ax = ( dx >= 0 ) ? dx : -dx;
    ay = ( dy >= 0 ) ? dy : -dy;

    if ( ay * 12 < ax )
    {
      /* |dy| <<< |dx|  means a near-horizontal segment */
      result = ( dx >= 0 ) ? PSH2_DIR_RIGHT : PSH2_DIR_LEFT;
    }
    else if ( ax * 12 < ay )
    {
      /* |dx| <<< |dy|  means a near-vertical segment */
      result = ( dy >= 0 ) ? PSH2_DIR_UP : PSH2_DIR_DOWN;
    }

    return result;
  }


  static FT_Error
  psh2_glyph_init( PSH2_Glyph   glyph,
                   FT_Outline*  outline,
                   PS_Hints     ps_hints,
                   PSH_Globals  globals )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* clear all fields */
    memset( glyph, 0, sizeof ( *glyph ) );

    memory = globals->memory;

    /* allocate and setup points + contours arrays */
    if ( ALLOC_ARRAY( glyph->points,   outline->n_points,
                      PSH2_PointRec   )                     ||
         ALLOC_ARRAY( glyph->contours, outline->n_contours,
                      PSH2_ContourRec )                     )
      goto Exit;

    glyph->num_points   = outline->n_points;
    glyph->num_contours = outline->n_contours;

    {
      FT_UInt       first = 0, next, n;
      PSH2_Point    points  = glyph->points;
      PSH2_Contour  contour = glyph->contours;


      for ( n = 0; n < glyph->num_contours; n++ )
      {
        FT_Int      count;
        PSH2_Point  point;


        next  = outline->contours[n] + 1;
        count = next - first;

        contour->start = points + first;
        contour->count = (FT_UInt)count;

        if ( count > 0 )
        {
          point = points + first;

          point->prev    = points + next - 1;
          point->contour = contour;

          for ( ; count > 1; count-- )
          {
            point[0].next = point + 1;
            point[1].prev = point;
            point++;
            point->contour = contour;
          }
          point->next = points + first;
        }

        contour++;
        first = next;
      }
    }

    {
      PSH2_Point  points = glyph->points;
      PSH2_Point  point  = points;
      FT_Vector*  vec    = outline->points;
      FT_UInt     n;


      for ( n = 0; n < glyph->num_points; n++, point++ )
      {
        FT_Int  n_prev = point->prev - points;
        FT_Int  n_next = point->next - points;
        FT_Pos  dxi, dyi, dxo, dyo;


        if ( !( outline->tags[n] & FT_Curve_Tag_On ) )
          point->flags = PSH2_POINT_OFF;

        dxi = vec[n].x - vec[n_prev].x;
        dyi = vec[n].y - vec[n_prev].y;

        point->dir_in = (FT_Char)psh2_compute_dir( dxi, dyi );

        dxo = vec[n_next].x - vec[n].x;
        dyo = vec[n_next].y - vec[n].y;

        point->dir_out = (FT_Char)psh2_compute_dir( dxo, dyo );

        /* detect smooth points */
        if ( point->flags & PSH2_POINT_OFF )
          point->flags |= PSH2_POINT_SMOOTH;
        else if ( point->dir_in  != PSH2_DIR_NONE ||
                  point->dir_out != PSH2_DIR_NONE )
        {
          if ( point->dir_in == point->dir_out )
            point->flags |= PSH2_POINT_SMOOTH;
        }
        else
        {
          FT_Angle  angle_in, angle_out, diff;


          angle_in  = FT_Atan2( dxi, dyi );
          angle_out = FT_Atan2( dxo, dyo );

          diff = angle_in - angle_out;
          if ( diff < 0 )
            diff = -diff;

          if ( diff > FT_ANGLE_PI )
            diff = FT_ANGLE_2PI - diff;

          if ( diff < FT_ANGLE_PI / 16 )
            point->flags |= PSH2_POINT_SMOOTH;
        }
      }
    }

    glyph->memory  = memory;
    glyph->outline = outline;
    glyph->globals = globals;

    /* now deal with hints tables */
    error = psh2_hint_table_init( &glyph->hint_tables [0],
                                  &ps_hints->dimension[0].hints,
                                  &ps_hints->dimension[0].masks,
                                  &ps_hints->dimension[0].counters,
                                  memory );
    if ( error )
      goto Exit;

    error = psh2_hint_table_init( &glyph->hint_tables [1],
                                  &ps_hints->dimension[1].hints,
                                  &ps_hints->dimension[1].masks,
                                  &ps_hints->dimension[1].counters,
                                  memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  /* load outline point coordinates into hinter glyph */
  static void
  psh2_glyph_load_points( PSH2_Glyph  glyph,
                          FT_Int      dimension )
  {
    FT_Vector*  vec   = glyph->outline->points;
    PSH2_Point  point = glyph->points;
    FT_UInt     count = glyph->num_points;


    for ( ; count > 0; count--, point++, vec++ )
    {
      point->flags &= PSH2_POINT_OFF | PSH2_POINT_SMOOTH;
      point->hint   = 0;
      if ( dimension == 0 )
        point->org_u = vec->x;
      else
        point->org_u = vec->y;

#ifdef DEBUG_HINTER
      point->org_x = vec->x;
      point->org_y = vec->y;
#endif
    }
  }


  /* save hinted point coordinates back to outline */
  static void
  psh2_glyph_save_points( PSH2_Glyph  glyph,
                          FT_Int      dimension )
  {
    FT_UInt     n;
    PSH2_Point  point = glyph->points;
    FT_Vector*  vec   = glyph->outline->points;
    char*       tags  = glyph->outline->tags;


    for ( n = 0; n < glyph->num_points; n++ )
    {
      if ( dimension == 0 )
        vec[n].x = point->cur_u;
      else
        vec[n].y = point->cur_u;

      if ( psh2_point_is_strong( point ) )
        tags[n] |= (char)((dimension == 0) ? 32 : 64);

#ifdef DEBUG_HINTER
      if ( dimension == 0 )
      {
        point->cur_x   = point->cur_u;
        point->flags_x = point->flags;
      }
      else
      {
        point->cur_y   = point->cur_u;
        point->flags_y = point->flags;
      }
#endif
      point++;
    }
  }


#define PSH2_STRONG_THRESHOLD  10


  static void
  psh2_hint_table_find_strong_point( PSH2_Hint_Table  table,
                                     PSH2_Point       point,
                                     FT_Int           major_dir )
  {
    PSH2_Hint*   sort      = table->sort;
    FT_UInt      num_hints = table->num_hints;


    for ( ; num_hints > 0; num_hints--, sort++ )
    {
      PSH2_Hint  hint = sort[0];


      if ( ABS( point->dir_in )  == major_dir ||
           ABS( point->dir_out ) == major_dir )
      {
        FT_Pos  d;


        d = point->org_u - hint->org_pos;
        if ( ABS( d ) < PSH2_STRONG_THRESHOLD )
        {
        Is_Strong:
          psh2_point_set_strong( point );
          point->hint = hint;
          break;
        }

        d -= hint->org_len;
        if ( ABS( d ) < PSH2_STRONG_THRESHOLD )
          goto Is_Strong;
      }

#if 1
      if ( point->org_u >= hint->org_pos &&
           point->org_u <= hint->org_pos + hint->org_len &&
           psh2_point_is_extremum( point ) )
      {
        /* attach to hint, but don't mark as strong */
        point->hint = hint;
        break;
      }
#endif
    }
  }


  /* find strong points in a glyph */
  static void
  psh2_glyph_find_strong_points( PSH2_Glyph  glyph,
                                 FT_Int      dimension )
  {
    /* a point is strong if it is located on a stem                   */
    /* edge and has an "in" or "out" tangent to the hint's direction  */
    {
      PSH2_Hint_Table  table     = &glyph->hint_tables[dimension];
      PS_Mask          mask      = table->hint_masks->masks;
      FT_UInt          num_masks = table->hint_masks->num_masks;
      FT_UInt          first     = 0;
      FT_Int           major_dir = dimension == 0 ? PSH2_DIR_UP : PSH2_DIR_RIGHT;


      /* process secondary hints to "selected" points */
      if ( num_masks > 1 )
      {
        mask++;
        for ( ; num_masks > 1; num_masks--, mask++ )
        {
          FT_UInt  next;
          FT_Int   count;


          next  = mask->end_point;
          count = next - first;
          if ( count > 0 )
          {
            PSH2_Point  point = glyph->points + first;


            psh2_hint_table_activate_mask( table, mask );

            for ( ; count > 0; count--, point++ )
              psh2_hint_table_find_strong_point( table, point, major_dir );
          }
          first = next;
        }
      }

      /* process primary hints for all points */
      if ( num_masks == 1 )
      {
        FT_UInt     count = glyph->num_points;
        PSH2_Point  point = glyph->points;


        psh2_hint_table_activate_mask( table, table->hint_masks->masks );
        for ( ; count > 0; count--, point++ )
        {
          if ( !psh2_point_is_strong( point ) )
            psh2_hint_table_find_strong_point( table, point, major_dir );
        }
      }

      /* now, certain points may have been attached to hint and */
      /* not marked as strong; update their flags then          */
      {
        FT_UInt     count = glyph->num_points;
        PSH2_Point  point = glyph->points;


        for ( ; count > 0; count--, point++ )
          if ( point->hint && !psh2_point_is_strong( point ) )
            psh2_point_set_strong( point );
      }
    }
  }


  /* interpolate strong points with the help of hinted coordinates */
  static void
  psh2_glyph_interpolate_strong_points( PSH2_Glyph  glyph,
                                        FT_Int      dimension )
  {
    PSH_Dimension    dim   = &glyph->globals->dimension[dimension];
    FT_Fixed         scale = dim->scale_mult;


    {
      FT_UInt     count = glyph->num_points;
      PSH2_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
      {
        PSH2_Hint  hint = point->hint;


        if ( hint )
        {
          FT_Pos  delta;


          delta = point->org_u - hint->org_pos;

          if ( delta <= 0 )
            point->cur_u = hint->cur_pos + FT_MulFix( delta, scale );

          else if ( delta >= hint->org_len )
            point->cur_u = hint->cur_pos + hint->cur_len +
                           FT_MulFix( delta - hint->org_len, scale );

          else if ( hint->org_len > 0 )
            point->cur_u = hint->cur_pos +
                           FT_MulDiv( delta, hint->cur_len, hint->org_len );
          else
            point->cur_u = hint->cur_pos;

          psh2_point_set_fitted( point );
        }
      }
    }
  }


  static void
  psh2_glyph_interpolate_normal_points( PSH2_Glyph  glyph,
                                        FT_Int      dimension )
  {
#if 1
    PSH_Dimension    dim   = &glyph->globals->dimension[dimension];
    FT_Fixed         scale = dim->scale_mult;


    /* first technique: a point is strong if it is a local extrema */
    {
      FT_UInt     count = glyph->num_points;
      PSH2_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
      {
        if ( psh2_point_is_strong( point ) )
          continue;

        /* sometimes, some local extremas are smooth points */
        if ( psh2_point_is_smooth( point ) )
        {
          if ( point->dir_in == PSH2_DIR_NONE  ||
               point->dir_in != point->dir_out )
            continue;

          if ( !psh2_point_is_extremum( point ) )
            continue;

          point->flags &= ~PSH2_POINT_SMOOTH;
        }

        /* find best enclosing point coordinates */
        {
          PSH2_Point  before = 0;
          PSH2_Point  after  = 0;

          FT_Pos      diff_before = -32000;
          FT_Pos      diff_after  =  32000;
          FT_Pos      u = point->org_u;

          FT_Int      count2 = glyph->num_points;
          PSH2_Point  cur    = glyph->points;


          for ( ; count2 > 0; count2--, cur++ )
          {
            if ( psh2_point_is_strong( cur ) )
            {
              FT_Pos   diff = cur->org_u - u;;


              if ( diff <= 0 )
              {
                if ( diff > diff_before )
                {
                  diff_before = diff;
                  before      = cur;
                }
              }
              else if ( diff >= 0 )
              {
                if ( diff < diff_after )
                {
                  diff_after = diff;
                  after      = cur;
                }
              }
            }
          }

          if ( !before )
          {
            if ( !after )
              continue;

            /* we are before the first strong point coordinate; */
            /* simply translate the point                       */
            point->cur_u = after->cur_u +
                           FT_MulFix( point->org_u - after->org_u, scale );
          }
          else if ( !after )
          {
            /* we are after the last strong point coordinate; */
            /* simply translate the point                     */
            point->cur_u = before->cur_u +
                           FT_MulFix( point->org_u - before->org_u, scale );
          }
          else
          {
            if ( diff_before == 0 )
              point->cur_u = before->cur_u;

            else if ( diff_after == 0 )
              point->cur_u = after->cur_u;

            else
              point->cur_u = before->cur_u +
                             FT_MulDiv( u - before->org_u,
                                        after->cur_u - before->cur_u,
                                        after->org_u - before->org_u );
          }

          psh2_point_set_fitted( point );
        }
      }
    }
#endif
  }


  /* interpolate other points */
  static void
  psh2_glyph_interpolate_other_points( PSH2_Glyph  glyph,
                                       FT_Int      dimension )
  {
    PSH_Dimension dim          = &glyph->globals->dimension[dimension];
    FT_Fixed      scale        = dim->scale_mult;
    FT_Fixed      delta        = dim->scale_delta;
    PSH2_Contour  contour      = glyph->contours;
    FT_UInt       num_contours = glyph->num_contours;


    for ( ; num_contours > 0; num_contours--, contour++ )
    {
      PSH2_Point   start = contour->start;
      PSH2_Point   first, next, point;
      FT_UInt      fit_count;


      /* count the number of strong points in this contour */
      next      = start + contour->count;
      fit_count = 0;
      first     = 0;

      for ( point = start; point < next; point++ )
        if ( psh2_point_is_fitted( point ) )
        {
          if ( !first )
            first = point;

          fit_count++;
        }

      /* if there are less than 2 fitted points in the contour, we */
      /* simply scale and eventually translate the contour points  */
      if ( fit_count < 2 )
      {
        if ( fit_count == 1 )
          delta = first->cur_u - FT_MulFix( first->org_u, scale );

        for ( point = start; point < next; point++ )
          if ( point != first )
            point->cur_u = FT_MulFix( point->org_u, scale ) + delta;

        goto Next_Contour;
      }

      /* there are more than 2 strong points in this contour; we */
      /* need to interpolate weak points between them            */
      start = first;
      do
      {
        point = first;

        /* skip consecutive fitted points */
        for (;;)
        {
          next = first->next;
          if ( next == start )
            goto Next_Contour;

          if ( !psh2_point_is_fitted( next ) )
            break;

          first = next;
        }

        /* find next fitted point after unfitted one */
        for (;;)
        {
          next = next->next;
          if ( psh2_point_is_fitted( next ) )
            break;
        }

        /* now interpolate between them */
        {
          FT_Pos    org_a, org_ab, cur_a, cur_ab;
          FT_Pos    org_c, org_ac, cur_c;
          FT_Fixed  scale_ab;


          if ( first->org_u <= next->org_u )
          {
            org_a  = first->org_u;
            cur_a  = first->cur_u;
            org_ab = next->org_u - org_a;
            cur_ab = next->cur_u - cur_a;
          }
          else
          {
            org_a  = next->org_u;
            cur_a  = next->cur_u;
            org_ab = first->org_u - org_a;
            cur_ab = first->cur_u - cur_a;
          }

          scale_ab = 0x10000L;
          if ( org_ab > 0 )
            scale_ab = FT_DivFix( cur_ab, org_ab );

          point = first->next;
          do
          {
            org_c  = point->org_u;
            org_ac = org_c - org_a;

            if ( org_ac <= 0 )
            {
              /* on the left of the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale );
            }
            else if ( org_ac >= org_ab )
            {
              /* on the right on the interpolation zone */
              cur_c = cur_a + cur_ab + FT_MulFix( org_ac - org_ab, scale );
            }
            else
            {
              /* within the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale_ab );
            }

            point->cur_u = cur_c;

            point = point->next;

          } while ( point != next );
        }

        /* keep going until all points in the contours have been processed */
        first = next;

      } while ( first != start );

    Next_Contour:
      ;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     HIGH-LEVEL INTERFACE                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_Error
  ps2_hints_apply( PS_Hints     ps_hints,
                   FT_Outline*  outline,
                   PSH_Globals  globals )
  {
    PSH2_GlyphRec  glyphrec;
    PSH2_Glyph     glyph = &glyphrec;
    FT_Error       error;
    FT_Memory      memory;
    FT_Int         dimension;

    memory = globals->memory;

#ifdef DEBUG_HINTER
    if ( ps2_debug_glyph )
    {
      psh2_glyph_done( ps2_debug_glyph );
      FREE( ps2_debug_glyph );
    }

    if ( ALLOC( glyph, sizeof ( *glyph ) ) )
      return error;

    ps2_debug_glyph = glyph;
#endif

    error = psh2_glyph_init( glyph, outline, ps_hints, globals );
    if ( error )
      goto Exit;

    for ( dimension = 0; dimension < 2; dimension++ )
    {
      /* load outline coordinates into glyph */
      psh2_glyph_load_points( glyph, dimension );

      /* compute aligned stem/hints positions */
      psh2_hint_table_align_hints( &glyph->hint_tables[dimension],
                                   glyph->globals,
                                   dimension );

      /* find strong points, align them, then interpolate others */
      psh2_glyph_find_strong_points( glyph, dimension );
      psh2_glyph_interpolate_strong_points( glyph, dimension );
      psh2_glyph_interpolate_normal_points( glyph, dimension );
      psh2_glyph_interpolate_other_points( glyph, dimension );

      /* save hinted coordinates back to outline */
      psh2_glyph_save_points( glyph, dimension );
    }

  Exit:
#ifndef DEBUG_HINTER
    psh2_glyph_done( glyph );
#endif
    return error;
  }


/* END */
