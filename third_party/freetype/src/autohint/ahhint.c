/***************************************************************************/
/*                                                                         */
/*  ahhint.c                                                               */
/*                                                                         */
/*    Glyph hinter (body).                                                 */
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


#include <ft2build.h>
#include "ahhint.h"
#include "ahglyph.h"
#include "ahangles.h"
#include "aherrors.h"
#include FT_OUTLINE_H


#define FACE_GLOBALS( face )  ((AH_Face_Globals*)(face)->autohint.data)

#define AH_USE_IUP


  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****   Hinting routines                                              ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/


  /* snap a given width in scaled coordinates to one of the */
  /* current standard widths                                */
  static FT_Pos
  ah_snap_width( FT_Pos*  widths,
                 FT_Int   count,
                 FT_Pos   width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n];
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    if ( width >= reference )
    {
      width -= 0x21;
      if ( width < reference )
        width = reference;
    }
    else
    {
      width += 0x21;
      if ( width > reference )
        width = reference;
    }

    return width;
  }


  /* align one stem edge relative to the previous stem edge */
  static void
  ah_align_linked_edge( AH_Hinter*  hinter,
                        AH_Edge*    base_edge,
                        AH_Edge*    stem_edge,
                        int         vertical )
  {
    FT_Pos       dist    = stem_edge->opos - base_edge->opos;
    AH_Globals*  globals = &hinter->globals->scaled;
    FT_Pos       sign    = 1;


    if ( dist < 0 )
    {
      dist = -dist;
      sign = -1;
    }

    if ( vertical )
    {
      dist = ah_snap_width( globals->heights, globals->num_heights, dist );

      /* in the case of vertical hinting, always round */
      /* the stem heights to integer pixels            */
      if ( dist >= 64 )
        dist = ( dist + 16 ) & -64;
      else
        dist = 64;
    }
    else
    {
      dist = ah_snap_width( globals->widths,  globals->num_widths, dist );

      if ( hinter->flags & ah_hinter_monochrome )
      {
        /* monochrome horizontal hinting: snap widths to integer pixels */
        /* with a different threshold                                   */
        if ( dist < 64 )
          dist = 64;
        else
          dist = ( dist + 32 ) & -64;
      }
      else
      {
        /* for horizontal anti-aliased hinting, we adopt a more subtle */
        /* approach: we strengthen small stems, round stems whose size */
        /* is between 1 and 2 pixels to an integer, otherwise nothing  */
        if ( dist < 48 )
          dist = ( dist + 64 ) >> 1;

        else if ( dist < 128 )
          dist = ( dist + 42 ) & -64;
        else
          /* XXX: round otherwise, prevent color fringes in LCD mode */
          dist = ( dist + 32 ) & -64;
      }
    }

    stem_edge->pos = base_edge->pos + sign * dist;
  }


  static void
  ah_align_serif_edge( AH_Hinter*  hinter,
                       AH_Edge*    base,
                       AH_Edge*    serif,
                       int         vertical )
  {
    FT_Pos  dist;
    FT_Pos  sign = 1;

    UNUSED( hinter );


    dist = serif->opos - base->opos;
    if ( dist < 0 )
    {
      dist = -dist;
      sign = -1;
    }

    /* do not strengthen serifs */
    if ( base->flags & ah_edge_done )
    {
      if ( dist >= 64 )
        dist = (dist+8) & -64;

      else if ( dist <= 32 && !vertical )
        dist = ( dist + 33 ) >> 1;
      else
        dist = 0;
    }

    serif->pos = base->pos + sign * dist;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       E D G E   H I N T I N G                                   ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* Another alternative edge hinting algorithm */
  static void
  ah_hint_edges_3( AH_Hinter*  hinter )
  {
    AH_Edge*     edges;
    AH_Edge*     edge_limit;
    AH_Outline*  outline = hinter->glyph;
    FT_Int       dimension;


    edges      = outline->horz_edges;
    edge_limit = edges + outline->num_hedges;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge*  edge;
      AH_Edge*  anchor = 0;
      int       has_serifs = 0;


      if ( ah_debug_disable_vert && !dimension )
        goto Next_Dimension;

      if ( ah_debug_disable_horz && dimension )
        goto Next_Dimension;

      /* we begin by aligning all stems relative to the blue zone */
      /* if needed -- that's only for horizontal edges            */
      if ( dimension )
      {
        for ( edge = edges; edge < edge_limit; edge++ )
        {
          FT_Pos*  blue;
          AH_Edge  *edge1, *edge2;


          if ( edge->flags & ah_edge_done )
            continue;

          blue  = edge->blue_edge;
          edge1 = 0;
          edge2 = edge->link;

          if ( blue )
          {
            edge1 = edge;
          }
          else if (edge2 && edge2->blue_edge)
          {
            blue  = edge2->blue_edge;
            edge1 = edge2;
            edge2 = edge;
          }

          if ( !edge1 )
            continue;

          edge1->pos    = blue[0];
          edge1->flags |= ah_edge_done;

          if ( edge2 && !edge2->blue_edge )
          {
            ah_align_linked_edge( hinter, edge1, edge2, dimension );
            edge2->flags |= ah_edge_done;
          }

          if ( !anchor )
            anchor = edge;
        }
      }

      /* now, we will align all stem edges, trying to maintain the */
      /* relative order of stems in the glyph..                    */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AH_Edge  *edge2;


        if ( edge->flags & ah_edge_done )
          continue;

        /* skip all non-stem edges */
        edge2 = edge->link;
        if ( !edge2 )
        {
          has_serifs++;
          continue;
        }

        /* now, align the stem */

        /* this should not happen, but it's better to be safe.. */
        if ( edge2->blue_edge || edge2 < edge )
        {

#if 0
          printf( "strange blue alignement, edge %d to %d\n",
                  edge - edges, edge2 - edges );
#endif

          ah_align_linked_edge( hinter, edge2, edge, dimension );
          edge->flags |= ah_edge_done;
          continue;
        }

        {
          FT_Bool  min = 0;
          FT_Pos   delta;

          if ( !anchor )
          {
            edge->pos = ( edge->opos + 32 ) & -64;
            anchor    = edge;
          }
          else
            edge->pos = anchor->pos +
                        ( ( edge->opos - anchor->opos + 32 ) & -64 );

          edge->flags |= ah_edge_done;

          if ( edge > edges && edge->pos < edge[-1].pos )
          {
            edge->pos = edge[-1].pos;
            min       = 1;
          }

          ah_align_linked_edge( hinter, edge, edge2, dimension );
          delta = 0;
          if ( edge2 + 1 < edge_limit        &&
               edge2[1].flags & ah_edge_done )
            delta = edge2[1].pos - edge2->pos;

          if ( delta < 0 )
          {
            edge2->pos += delta;
            if ( !min )
              edge->pos += delta;
          }
          edge2->flags |= ah_edge_done;
        }
      }

      if ( !has_serifs )
        goto Next_Dimension;

      /* now, hint the remaining edges (serifs and single) in order */
      /* to complete our processing                                 */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        if ( edge->flags & ah_edge_done )
          continue;

        if ( edge->serif )
        {
          ah_align_serif_edge( hinter, edge->serif, edge, dimension );
        }
        else if ( !anchor )
        {
          edge->pos = ( edge->opos + 32 ) & -64;
          anchor    = edge;
        }
        else
          edge->pos = anchor->pos +
                      ( ( edge->opos-anchor->opos + 32 ) & -64 );

        edge->flags |= ah_edge_done;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;

        if ( edge + 1 < edge_limit        &&
             edge[1].flags & ah_edge_done &&
             edge->pos > edge[1].pos      )
          edge->pos = edge[1].pos;
      }

    Next_Dimension:
      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
    }
  }


  FT_LOCAL_DEF void
  ah_hinter_hint_edges( AH_Hinter*  hinter,
                        FT_Bool     no_horz_edges,
                        FT_Bool     no_vert_edges )
  {
#if 0
    ah_debug_disable_horz = no_horz_edges;
    ah_debug_disable_vert = no_vert_edges;
#else
    FT_UNUSED( no_horz_edges );
    FT_UNUSED( no_vert_edges );    
#endif
    /* AH_Interpolate_Blue_Edges( hinter ); -- doesn't seem to help      */
    /* reduce the problem of the disappearing eye in the `e' of Times... */
    /* also, creates some artifacts near the blue zones?                 */
    {
      ah_hint_edges_3( hinter );

#if 0
      /* outline optimizer removed temporarily */
      if ( hinter->flags & ah_hinter_optimize )
      {
        AH_Optimizer  opt;


        if ( !AH_Optimizer_Init( &opt, hinter->glyph, hinter->memory ) )
        {
          AH_Optimizer_Compute( &opt );
          AH_Optimizer_Done( &opt );
        }
      }
#endif

    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       P O I N T   H I N T I N G                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static void
  ah_hinter_align_edge_points( AH_Hinter*  hinter )
  {
    AH_Outline*  outline = hinter->glyph;
    AH_Edge*     edges;
    AH_Edge*     edge_limit;
    FT_Int       dimension;


    edges      = outline->horz_edges;
    edge_limit = edges + outline->num_hedges;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Edge*   edge;


      edge = edges;
      for ( ; edge < edge_limit; edge++ )
      {
        /* move the points of each segment     */
        /* in each edge to the edge's position */
        AH_Segment*  seg = edge->first;


        do
        {
          AH_Point*  point = seg->first;


          for (;;)
          {
            if ( dimension )
            {
              point->y      = edge->pos;
              point->flags |= ah_flag_touch_y;
            }
            else
            {
              point->x      = edge->pos;
              point->flags |= ah_flag_touch_x;
            }

            if ( point == seg->last )
              break;

            point = point->next;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );
      }

      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
    }
  }


  /* hint the strong points -- this is equivalent to the TrueType `IP' */
  static void
  ah_hinter_align_strong_points( AH_Hinter*  hinter )
  {
    AH_Outline*  outline = hinter->glyph;
    FT_Int       dimension;
    AH_Edge*     edges;
    AH_Edge*     edge_limit;
    AH_Point*    points;
    AH_Point*    point_limit;
    AH_Flags     touch_flag;


    points      = outline->points;
    point_limit = points + outline->num_points;

    edges       = outline->horz_edges;
    edge_limit  = edges + outline->num_hedges;
    touch_flag  = ah_flag_touch_y;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Point*  point;
      AH_Edge*   edge;


      if ( edges < edge_limit )
        for ( point = points; point < point_limit; point++ )
        {
          FT_Pos  u, ou, fu;  /* point position */
          FT_Pos  delta;


          if ( point->flags & touch_flag )
            continue;

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
          /* if this point is candidate to weak interpolation, we will  */
          /* interpolate it after all strong points have been processed */
          if ( point->flags & ah_flag_weak_interpolation )
            continue;
#endif

          if ( dimension )
          {
            u  = point->fy;
            ou = point->oy;
          }
          else
          {
            u  = point->fx;
            ou = point->ox;
          }

          fu = u;

          /* is the point before the first edge? */
          edge  = edges;
          delta = edge->fpos - u;
          if ( delta >= 0 )
          {
            u = edge->pos - ( edge->opos - ou );
            goto Store_Point;
          }

          /* is the point after the last edge ? */
          edge  = edge_limit - 1;
          delta = u - edge->fpos;
          if ( delta >= 0 )
          {
            u = edge->pos + ( ou - edge->opos );
            goto Store_Point;
          }

          /* otherwise, interpolate the point in between */
          {
            AH_Edge*  before = 0;
            AH_Edge*  after  = 0;


            for ( edge = edges; edge < edge_limit; edge++ )
            {
              if ( u == edge->fpos )
              {
                u = edge->pos;
                goto Store_Point;
              }
              if ( u < edge->fpos )
                break;
              before = edge;
            }

            for ( edge = edge_limit - 1; edge >= edges; edge-- )
            {
              if ( u == edge->fpos )
              {
                u = edge->pos;
                goto Store_Point;
              }
              if ( u > edge->fpos )
                break;
              after = edge;
            }

            /* assert( before && after && before != after ) */
            u = before->pos + FT_MulDiv( fu - before->fpos,
                                         after->pos - before->pos,
                                         after->fpos - before->fpos );
          }

        Store_Point:

          /* save the point position */
          if ( dimension )
            point->y = u;
          else
            point->x = u;

          point->flags |= touch_flag;
        }

      edges      = outline->vert_edges;
      edge_limit = edges + outline->num_vedges;
      touch_flag = ah_flag_touch_x;
    }
  }


#ifndef AH_OPTION_NO_WEAK_INTERPOLATION

  static void
  ah_iup_shift( AH_Point*  p1,
                AH_Point*  p2,
                AH_Point*  ref )
  {
    AH_Point*  p;
    FT_Pos     delta = ref->u - ref->v;


    for ( p = p1; p < ref; p++ )
      p->u = p->v + delta;

    for ( p = ref + 1; p <= p2; p++ )
      p->u = p->v + delta;
  }


  static void
  ah_iup_interp( AH_Point*  p1,
                 AH_Point*  p2,
                 AH_Point*  ref1,
                 AH_Point*  ref2 )
  {
    AH_Point*  p;
    FT_Pos     u;
    FT_Pos     v1 = ref1->v;
    FT_Pos     v2 = ref2->v;
    FT_Pos     d1 = ref1->u - v1;
    FT_Pos     d2 = ref2->u - v2;


    if ( p1 > p2 )
      return;

    if ( v1 == v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else
          u += d2;

        p->u = u;
      }
      return;
    }

    if ( v1 < v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else if ( u >= v2 )
          u += d2;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
    else
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v2 )
          u += d2;
        else if ( u >= v1 )
          u += d1;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
  }


  /* interpolate weak points -- this is equivalent to the TrueType `IUP' */
  static void
  ah_hinter_align_weak_points( AH_Hinter*  hinter )
  {
    AH_Outline*  outline = hinter->glyph;
    FT_Int       dimension;
    AH_Point*    points;
    AH_Point*    point_limit;
    AH_Point**   contour_limit;
    AH_Flags     touch_flag;


    points      = outline->points;
    point_limit = points + outline->num_points;

    /* PASS 1: Move segment points to edge positions */

    touch_flag = ah_flag_touch_y;

    contour_limit = outline->contours + outline->num_contours;

    ah_setup_uv( outline, ah_uv_oy );

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Point*   point;
      AH_Point*   end_point;
      AH_Point*   first_point;
      AH_Point**  contour;


      point   = points;
      contour = outline->contours;

      for ( ; contour < contour_limit; contour++ )
      {
        point       = *contour;
        end_point   = point->prev;
        first_point = point;

        while ( point <= end_point && !( point->flags & touch_flag ) )
          point++;

        if ( point <= end_point )
        {
          AH_Point*  first_touched = point;
          AH_Point*  cur_touched   = point;


          point++;
          while ( point <= end_point )
          {
            if ( point->flags & touch_flag )
            {
              /* we found two successive touched points; we interpolate */
              /* all contour points between them                        */
              ah_iup_interp( cur_touched + 1, point - 1,
                             cur_touched, point );
              cur_touched = point;
            }
            point++;
          }

          if ( cur_touched == first_touched )
          {
            /* this is a special case: only one point was touched in the */
            /* contour; we thus simply shift the whole contour           */
            ah_iup_shift( first_point, end_point, cur_touched );
          }
          else
          {
            /* now interpolate after the last touched point to the end */
            /* of the contour                                          */
            ah_iup_interp( cur_touched + 1, end_point,
                           cur_touched, first_touched );

            /* if the first contour point isn't touched, interpolate */
            /* from the contour start to the first touched point     */
            if ( first_touched > points )
              ah_iup_interp( first_point, first_touched - 1,
                             cur_touched, first_touched );
          }
        }
      }

      /* now save the interpolated values back to x/y */
      if ( dimension )
      {
        for ( point = points; point < point_limit; point++ )
          point->y = point->u;

        touch_flag = ah_flag_touch_x;
        ah_setup_uv( outline, ah_uv_ox );
      }
      else
      {
        for ( point = points; point < point_limit; point++ )
          point->x = point->u;

        break;  /* exit loop */
      }
    }
  }

#endif /* !AH_OPTION_NO_WEAK_INTERPOLATION */


  FT_LOCAL_DEF void
  ah_hinter_align_points( AH_Hinter*  hinter )
  {
    ah_hinter_align_edge_points( hinter );

#ifndef AH_OPTION_NO_STRONG_INTERPOLATION
    ah_hinter_align_strong_points( hinter );
#endif

#ifndef AH_OPTION_NO_WEAK_INTERPOLATION
    ah_hinter_align_weak_points( hinter );
#endif
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****       H I N T E R   O B J E C T   M E T H O D S                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /* scale and fit the global metrics */
  static void
  ah_hinter_scale_globals( AH_Hinter*  hinter,
                           FT_Fixed    x_scale,
                           FT_Fixed    y_scale )
  {
    FT_Int            n;
    AH_Face_Globals*  globals = hinter->globals;
    AH_Globals*       design = &globals->design;
    AH_Globals*       scaled = &globals->scaled;


    /* copy content */
    *scaled = *design;

    /* scale the standard widths & heights */
    for ( n = 0; n < design->num_widths; n++ )
      scaled->widths[n] = FT_MulFix( design->widths[n], x_scale );

    for ( n = 0; n < design->num_heights; n++ )
      scaled->heights[n] = FT_MulFix( design->heights[n], y_scale );

    /* scale the blue zones */
    for ( n = 0; n < ah_blue_max; n++ )
    {
      FT_Pos  delta, delta2;


      delta = design->blue_shoots[n] - design->blue_refs[n];
      delta2 = delta;
      if ( delta < 0 )
        delta2 = -delta2;
      delta2 = FT_MulFix( delta2, y_scale );

      if ( delta2 < 32 )
        delta2 = 0;
      else if ( delta2 < 64 )
        delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & -32 );
      else
        delta2 = ( delta2 + 32 ) & -64;

      if ( delta < 0 )
        delta2 = -delta2;

      scaled->blue_refs[n] =
        ( FT_MulFix( design->blue_refs[n], y_scale ) + 32 ) & -64;
      scaled->blue_shoots[n] = scaled->blue_refs[n] + delta2;
    }

    globals->x_scale = x_scale;
    globals->y_scale = y_scale;
  }


  static void
  ah_hinter_align( AH_Hinter*  hinter )
  {
    ah_hinter_align_edge_points( hinter );
    ah_hinter_align_points( hinter );
  }


  /* finalize a hinter object */
  FT_LOCAL_DEF void
  ah_hinter_done( AH_Hinter*  hinter )
  {
    if ( hinter )
    {
      FT_Memory  memory = hinter->memory;


      ah_loader_done( hinter->loader );
      ah_outline_done( hinter->glyph );

      /* note: the `globals' pointer is _not_ owned by the hinter */
      /*       but by the current face object, we don't need to   */
      /*       release it                                         */
      hinter->globals = 0;
      hinter->face    = 0;

      FREE( hinter );
    }
  }


  /* create a new empty hinter object */
  FT_LOCAL_DEF FT_Error
  ah_hinter_new( FT_Library   library,
                 AH_Hinter**  ahinter )
  {
    AH_Hinter*  hinter = 0;
    FT_Memory   memory = library->memory;
    FT_Error    error;


    *ahinter = 0;

    /* allocate object */
    if ( ALLOC( hinter, sizeof ( *hinter ) ) )
      goto Exit;

    hinter->memory = memory;
    hinter->flags  = 0;

    /* allocate outline and loader */
    error = ah_outline_new( memory, &hinter->glyph )  ||
            ah_loader_new ( memory, &hinter->loader ) ||
            ah_loader_create_extra( hinter->loader );
    if ( error )
      goto Exit;

    *ahinter = hinter;

  Exit:
    if ( error )
      ah_hinter_done( hinter );

    return error;
  }


  /* create a face's autohint globals */
  FT_LOCAL_DEF FT_Error
  ah_hinter_new_face_globals( AH_Hinter*   hinter,
                              FT_Face      face,
                              AH_Globals*  globals )
  {
    FT_Error          error;
    FT_Memory         memory = hinter->memory;
    AH_Face_Globals*  face_globals;


    if ( ALLOC( face_globals, sizeof ( *face_globals ) ) )
      goto Exit;

    hinter->face    = face;
    hinter->globals = face_globals;

    if ( globals )
      face_globals->design = *globals;
    else
      ah_hinter_compute_globals( hinter );

    face->autohint.data      = face_globals;
    face->autohint.finalizer = (FT_Generic_Finalizer)
                                 ah_hinter_done_face_globals;
    face_globals->face       = face;

  Exit:
    return error;
  }


  /* discard a face's autohint globals */
  FT_LOCAL_DEF void
  ah_hinter_done_face_globals( AH_Face_Globals*  globals )
  {
    FT_Face    face   = globals->face;
    FT_Memory  memory = face->memory;


    FREE( globals );
  }


  static FT_Error
  ah_hinter_load( AH_Hinter*  hinter,
                  FT_UInt     glyph_index,
                  FT_UInt     load_flags,
                  FT_UInt     depth )
  {
    FT_Face           face     = hinter->face;
    FT_GlyphSlot      slot     = face->glyph;
    FT_Slot_Internal  internal = slot->internal;
    FT_Fixed          x_scale  = face->size->metrics.x_scale;
    FT_Fixed          y_scale  = face->size->metrics.y_scale;
    FT_Error          error;
    AH_Outline*       outline  = hinter->glyph;
    AH_Loader*        gloader  = hinter->loader;
    FT_Bool           no_horz_hints = FT_BOOL(
                        ( load_flags & AH_HINT_NO_HORZ_EDGES ) != 0 );
    FT_Bool           no_vert_hints = FT_BOOL(
                        ( load_flags & AH_HINT_NO_VERT_EDGES ) != 0 );


    /* load the glyph */
    error = FT_Load_Glyph( face, glyph_index, load_flags );
    if ( error )
      goto Exit;

    /* Set `hinter->transformed' after loading with FT_LOAD_NO_RECURSE. */
    hinter->transformed = internal->glyph_transformed;

    if ( hinter->transformed )
    {
      FT_Matrix  imatrix;

      imatrix              = internal->glyph_matrix;
      hinter->trans_delta  = internal->glyph_delta;
      hinter->trans_matrix = imatrix;

      FT_Matrix_Invert( &imatrix );
      FT_Vector_Transform( &hinter->trans_delta, &imatrix );
    }

    /* set linear horizontal metrics */
    slot->linearHoriAdvance = slot->metrics.horiAdvance;
    slot->linearVertAdvance = slot->metrics.vertAdvance;

    switch ( slot->format )
    {
    case ft_glyph_format_outline:

      /* translate glyph outline if we need to */
      if ( hinter->transformed )
      {
        FT_UInt     n     = slot->outline.n_points;
        FT_Vector*  point = slot->outline.points;


        for ( ; n > 0; point++, n-- )
        {
          point->x += hinter->trans_delta.x;
          point->y += hinter->trans_delta.y;
        }
      }

      /* copy the outline points in the loader's current                */
      /* extra points, which is used to keep original glyph coordinates */
      error = ah_loader_check_points( gloader, slot->outline.n_points + 2,
                                      slot->outline.n_contours );
      if ( error )
        goto Exit;

      MEM_Copy( gloader->current.extra_points, slot->outline.points,
                slot->outline.n_points * sizeof ( FT_Vector ) );

      MEM_Copy( gloader->current.outline.contours, slot->outline.contours,
                slot->outline.n_contours * sizeof ( short ) );

      MEM_Copy( gloader->current.outline.tags, slot->outline.tags,
                slot->outline.n_points * sizeof ( char ) );

      gloader->current.outline.n_points   = slot->outline.n_points;
      gloader->current.outline.n_contours = slot->outline.n_contours;

      /* compute original phantom points */
      hinter->pp1.x = 0;
      hinter->pp1.y = 0;
      hinter->pp2.x = FT_MulFix( slot->metrics.horiAdvance, x_scale );
      hinter->pp2.y = 0;

      /* be sure to check for spacing glyphs */
      if ( slot->outline.n_points == 0 )
        goto Hint_Metrics;

      /* now, load the slot image into the auto-outline, and run the */
      /* automatic hinting process                                   */
      error = ah_outline_load( outline, face );   /* XXX: change to slot */
      if ( error )
        goto Exit;

      /* perform feature detection */
      ah_outline_detect_features( outline );

      if ( !no_horz_hints )
      {
        ah_outline_compute_blue_edges( outline, hinter->globals );
        ah_outline_scale_blue_edges( outline, hinter->globals );
      }

      /* perform alignment control */
      ah_hinter_hint_edges( hinter, no_horz_hints, no_vert_hints );
      ah_hinter_align( hinter );

      /* now save the current outline into the loader's current table */
      ah_outline_save( outline, gloader );

      /* we now need to hint the metrics according to the change in */
      /* width/positioning that occured during the hinting process  */
      {
        FT_Pos    old_advance, old_rsb, old_lsb, new_lsb;
        AH_Edge*  edge1 = outline->vert_edges;     /* leftmost edge  */
        AH_Edge*  edge2 = edge1 +
                          outline->num_vedges - 1; /* rightmost edge */


        old_advance = hinter->pp2.x;
        old_rsb     = old_advance - edge2->opos;
        old_lsb     = edge1->opos;
        new_lsb     = edge1->pos;

        hinter->pp1.x = ( ( new_lsb    - old_lsb ) + 32 ) & -64;
        hinter->pp2.x = ( ( edge2->pos + old_rsb ) + 32 ) & -64;

        /* try to fix certain bad advance computations */
        if ( hinter->pp2.x + hinter->pp1.x == edge2->pos && old_rsb > 4 )
          hinter->pp2.x += 64;
      }

      /* good, we simply add the glyph to our loader's base */
      ah_loader_add( gloader );
      break;

    case ft_glyph_format_composite:
      {
        FT_UInt       nn, num_subglyphs = slot->num_subglyphs;
        FT_UInt       num_base_subgs, start_point;
        FT_SubGlyph*  subglyph;


        start_point   = gloader->base.outline.n_points;

        /* first of all, copy the subglyph descriptors in the glyph loader */
        error = ah_loader_check_subglyphs( gloader, num_subglyphs );
        if ( error )
          goto Exit;

        MEM_Copy( gloader->current.subglyphs, slot->subglyphs,
                  num_subglyphs * sizeof ( FT_SubGlyph ) );

        gloader->current.num_subglyphs = num_subglyphs;
        num_base_subgs = gloader->base.num_subglyphs;

        /* now, read each subglyph independently */
        for ( nn = 0; nn < num_subglyphs; nn++ )
        {
          FT_Vector  pp1, pp2;
          FT_Pos     x, y;
          FT_UInt    num_points, num_new_points, num_base_points;


          /* gloader.current.subglyphs can change during glyph loading due */
          /* to re-allocation -- we must recompute the current subglyph on */
          /* each iteration                                                */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          pp1 = hinter->pp1;
          pp2 = hinter->pp2;

          num_base_points = gloader->base.outline.n_points;

          error = ah_hinter_load( hinter, subglyph->index,
                                  load_flags, depth + 1 );
          if ( error )
            goto Exit;

          /* recompute subglyph pointer */
          subglyph = gloader->base.subglyphs + num_base_subgs + nn;

          if ( subglyph->flags & FT_SUBGLYPH_FLAG_USE_MY_METRICS )
          {
            pp1 = hinter->pp1;
            pp2 = hinter->pp2;
          }
          else
          {
            hinter->pp1 = pp1;
            hinter->pp2 = pp2;
          }

          num_points     = gloader->base.outline.n_points;
          num_new_points = num_points - num_base_points;

          /* now perform the transform required for this subglyph */

          if ( subglyph->flags & ( FT_SUBGLYPH_FLAG_SCALE    |
                                   FT_SUBGLYPH_FLAG_XY_SCALE |
                                   FT_SUBGLYPH_FLAG_2X2      ) )
          {
            FT_Vector*  cur   = gloader->base.outline.points +
                                num_base_points;
            FT_Vector*  org   = gloader->base.extra_points +
                                num_base_points;
            FT_Vector*  limit = cur + num_new_points;


            for ( ; cur < limit; cur++, org++ )
            {
              FT_Vector_Transform( cur, &subglyph->transform );
              FT_Vector_Transform( org, &subglyph->transform );
            }
          }

          /* apply offset */

          if ( !( subglyph->flags & FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES ) )
          {
            FT_Int      k = subglyph->arg1;
            FT_UInt     l = subglyph->arg2;
            FT_Vector*  p1;
            FT_Vector*  p2;


            if ( start_point + k >= num_base_points          ||
                               l >= (FT_UInt)num_new_points  )
            {
              error = AH_Err_Invalid_Composite;
              goto Exit;
            }

            l += num_base_points;

            /* for now, only use the current point coordinates     */
            /* we may consider another approach in the near future */
            p1 = gloader->base.outline.points + start_point + k;
            p2 = gloader->base.outline.points + start_point + l;

            x = p1->x - p2->x;
            y = p1->y - p2->y;
          }
          else
          {
            x = FT_MulFix( subglyph->arg1, x_scale );
            y = FT_MulFix( subglyph->arg2, y_scale );

            x = ( x + 32 ) & -64;
            y = ( y + 32 ) & -64;
          }

          {
            FT_Outline  dummy = gloader->base.outline;


            dummy.points  += num_base_points;
            dummy.n_points = (short)num_new_points;

            FT_Outline_Translate( &dummy, x, y );
          }
        }
      }
      break;

    default:
      /* we don't support other formats (yet?) */
      error = AH_Err_Unimplemented_Feature;
    }

  Hint_Metrics:
    if ( depth == 0 )
    {
      FT_BBox  bbox;


      /* transform the hinted outline if needed */
      if ( hinter->transformed )
        FT_Outline_Transform( &gloader->base.outline, &hinter->trans_matrix );

      /* we must translate our final outline by -pp1.x, and compute */
      /* the new metrics                                            */
      if ( hinter->pp1.x )
        FT_Outline_Translate( &gloader->base.outline, -hinter->pp1.x, 0 );

      FT_Outline_Get_CBox( &gloader->base.outline, &bbox );
      bbox.xMin &= -64;
      bbox.yMin &= -64;
      bbox.xMax  = ( bbox.xMax + 63 ) & -64;
      bbox.yMax  = ( bbox.yMax + 63 ) & -64;

      slot->metrics.width        = bbox.xMax - bbox.xMin;
      slot->metrics.height       = bbox.yMax - bbox.yMin;
      slot->metrics.horiBearingX = bbox.xMin;
      slot->metrics.horiBearingY = bbox.yMax;

      /* for mono-width fonts (like Andale, Courier, etc.), we need */
      /* to keep the original rounded advance width                 */
      if ( !FT_IS_FIXED_WIDTH( slot->face ) )
        slot->metrics.horiAdvance = hinter->pp2.x - hinter->pp1.x;
      else
        slot->metrics.horiAdvance = FT_MulFix( slot->metrics.horiAdvance,
                                               x_scale );

      slot->metrics.horiAdvance = ( slot->metrics.horiAdvance + 32 ) & -64;

      /* now copy outline into glyph slot */
      ah_loader_rewind( slot->internal->loader );
      error = ah_loader_copy_points( slot->internal->loader, gloader );
      if ( error )
        goto Exit;

      slot->outline = slot->internal->loader->base.outline;
      slot->format  = ft_glyph_format_outline;
    }

#ifdef DEBUG_HINTER
    ah_debug_hinter = hinter;
#endif

  Exit:
    return error;
  }


  /* load and hint a given glyph */
  FT_LOCAL_DEF FT_Error
  ah_hinter_load_glyph( AH_Hinter*    hinter,
                        FT_GlyphSlot  slot,
                        FT_Size       size,
                        FT_UInt       glyph_index,
                        FT_Int        load_flags )
  {
    FT_Face           face         = slot->face;
    FT_Error          error;
    FT_Fixed          x_scale      = size->metrics.x_scale;
    FT_Fixed          y_scale      = size->metrics.y_scale;
    AH_Face_Globals*  face_globals = FACE_GLOBALS( face );


    /* first of all, we need to check that we're using the correct face and */
    /* global hints to load the glyph                                       */
    if ( hinter->face != face || hinter->globals != face_globals )
    {
      hinter->face = face;
      if ( !face_globals )
      {
        error = ah_hinter_new_face_globals( hinter, face, 0 );
        if ( error )
          goto Exit;

      }
      hinter->globals = FACE_GLOBALS( face );
      face_globals    = FACE_GLOBALS( face );

    }

    /* now, we must check the current character pixel size to see if we */
    /* need to rescale the global metrics                               */
    if ( face_globals->x_scale != x_scale ||
         face_globals->y_scale != y_scale )
      ah_hinter_scale_globals( hinter, x_scale, y_scale );

    ah_loader_rewind( hinter->loader );

#if 1
    load_flags  = FT_LOAD_NO_SCALE
                | FT_LOAD_IGNORE_TRANSFORM ;
#else
    load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_RECURSE;
#endif

    error = ah_hinter_load( hinter, glyph_index, load_flags, 0 );

  Exit:
    return error;
  }


  /* retrieve a face's autohint globals for client applications */
  FT_LOCAL_DEF void
  ah_hinter_get_global_hints( AH_Hinter*  hinter,
                              FT_Face     face,
                              void**      global_hints,
                              long*       global_len )
  {
    AH_Globals*  globals = 0;
    FT_Memory    memory  = hinter->memory;
    FT_Error     error;


    /* allocate new master globals */
    if ( ALLOC( globals, sizeof ( *globals ) ) )
      goto Fail;

    /* compute face globals if needed */
    if ( !FACE_GLOBALS( face ) )
    {
      error = ah_hinter_new_face_globals( hinter, face, 0 );
      if ( error )
        goto Fail;
    }

    *globals      = FACE_GLOBALS( face )->design;
    *global_hints = globals;
    *global_len   = sizeof( *globals );

    return;

  Fail:
    FREE( globals );

    *global_hints = 0;
    *global_len   = 0;
  }


  FT_LOCAL_DEF void
  ah_hinter_done_global_hints( AH_Hinter*  hinter,
                               void*       global_hints )
  {
    FT_Memory  memory = hinter->memory;


    FREE( global_hints );
  }


/* END */
