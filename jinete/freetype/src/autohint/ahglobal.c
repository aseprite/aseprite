/***************************************************************************/
/*                                                                         */
/*  ahglobal.c                                                             */
/*                                                                         */
/*    Routines used to compute global metrics automatically (body).        */
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
#include FT_INTERNAL_DEBUG_H
#include "ahglobal.h"
#include "ahglyph.h"


#define MAX_TEST_CHARACTERS  12

  static
  const char*  blue_chars[ah_blue_max] =
  {
    "THEZOCQS",
    "HEZLOCUS",
    "xzroesc",
    "xzroesc",
    "pqgjy"
  };


  /* simple insertion sort */
  static void
  sort_values( FT_Int   count,
               FT_Pos*  table )
  {
    FT_Int  i, j, swap;


    for ( i = 1; i < count; i++ )
    {
      for ( j = i; j > 0; j-- )
      {
        if ( table[j] > table[j - 1] )
          break;

        swap         = table[j];
        table[j]     = table[j - 1];
        table[j - 1] = swap;
      }
    }
  }


  static FT_Error
  ah_hinter_compute_blues( AH_Hinter*  hinter )
  {
    AH_Blue       blue;
    AH_Globals*   globals = &hinter->globals->design;
    FT_Pos        flats [MAX_TEST_CHARACTERS];
    FT_Pos        rounds[MAX_TEST_CHARACTERS];
    FT_Int        num_flats;
    FT_Int        num_rounds;

    FT_Face       face;
    FT_GlyphSlot  glyph;
    FT_Error      error;
    FT_CharMap    charmap;


    face  = hinter->face;
    glyph = face->glyph;

    /* save current charmap */
    charmap = face->charmap;

    /* do we have a Unicode charmap in there? */
    error = FT_Select_Charmap( face, ft_encoding_unicode );
    if ( error )
      goto Exit;

    /* we compute the blues simply by loading each character from the */
    /* 'blue_chars[blues]' string, then compute its top-most and      */
    /* bottom-most points                                             */

    AH_LOG(( "blue zones computation\n" ));
    AH_LOG(( "------------------------------------------------\n" ));

    for ( blue = ah_blue_capital_top; blue < ah_blue_max; blue++ )
    {
      const char*  p     = blue_chars[blue];
      const char*  limit = p + MAX_TEST_CHARACTERS;
      FT_Pos       *blue_ref, *blue_shoot;


      AH_LOG(( "blue %3d: ", blue ));

      num_flats  = 0;
      num_rounds = 0;

      for ( ; p < limit; p++ )
      {
        FT_UInt     glyph_index;
        FT_Vector*  extremum;
        FT_Vector*  points;
        FT_Vector*  point_limit;
        FT_Vector*  point;
        FT_Bool     round;


        /* exit if we reach the end of the string */
        if ( !*p )
          break;

        AH_LOG(( "`%c'", *p ));

        /* load the character in the face -- skip unknown or empty ones */
        glyph_index = FT_Get_Char_Index( face, (FT_UInt)*p );
        if ( glyph_index == 0 )
          continue;

        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
        if ( error || glyph->outline.n_points <= 0 )
          continue;

        /* now compute min or max point indices and coordinates */
        points      = glyph->outline.points;
        point_limit = points + glyph->outline.n_points;
        point       = points;
        extremum    = point;
        point++;

        if ( AH_IS_TOP_BLUE( blue ) )
        {
          for ( ; point < point_limit; point++ )
            if ( point->y > extremum->y )
              extremum = point;
        }
        else
        {
          for ( ; point < point_limit; point++ )
            if ( point->y < extremum->y )
              extremum = point;
        }

        AH_LOG(( "%5d", (int)extremum->y ));

        /* now, check whether the point belongs to a straight or round  */
        /* segment; we first need to find in which contour the extremum */
        /* lies, then see its previous and next points                  */
        {
          FT_Int  index = (FT_Int)( extremum - points );
          FT_Int  n;
          FT_Int  first, last, prev, next, end;
          FT_Pos  dist;


          last  = -1;
          first = 0;

          for ( n = 0; n < glyph->outline.n_contours; n++ )
          {
            end = glyph->outline.contours[n];
            if ( end >= index )
            {
              last = end;
              break;
            }
            first = end + 1;
          }

          /* XXX: should never happen! */
          if ( last < 0 )
            continue;

          /* now look for the previous and next points that are not on the */
          /* same Y coordinate.  Threshold the `closeness'...              */

          prev = index;
          next = prev;

          do
          {
            if ( prev > first )
              prev--;
            else
              prev = last;

            dist = points[prev].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( prev != index );

          do
          {
            if ( next < last )
              next++;
            else
              next = first;

            dist = points[next].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( next != index );

          /* now, set the `round' flag depending on the segment's kind */
          round = FT_BOOL(
            FT_CURVE_TAG( glyph->outline.tags[prev] ) != FT_Curve_Tag_On ||
            FT_CURVE_TAG( glyph->outline.tags[next] ) != FT_Curve_Tag_On );

          AH_LOG(( "%c ", round ? 'r' : 'f' ));
        }

        if ( round )
          rounds[num_rounds++] = extremum->y;
        else
          flats[num_flats++] = extremum->y;
      }

      AH_LOG(( "\n" ));

      /* we have computed the contents of the `rounds' and `flats' tables, */
      /* now determine the reference and overshoot position of the blue;   */
      /* we simply take the median value after a simple short              */
      sort_values( num_rounds, rounds );
      sort_values( num_flats,  flats  );

      blue_ref   = globals->blue_refs + blue;
      blue_shoot = globals->blue_shoots + blue;
      if ( num_flats == 0 && num_rounds == 0 )
      {
        *blue_ref   = -10000;
        *blue_shoot = -10000;
      }
      else if ( num_flats == 0 )
      {
        *blue_ref   =
        *blue_shoot = rounds[num_rounds / 2];
      }
      else if ( num_rounds == 0 )
      {
        *blue_ref   =
        *blue_shoot = flats[num_flats / 2];
      }
      else
      {
        *blue_ref   = flats[num_flats / 2];
        *blue_shoot = rounds[num_rounds / 2];
      }

      /* there are sometimes problems: if the overshoot position of top     */
      /* zones is under its reference position, or the opposite for bottom  */
      /* zones.  We must thus check everything there and correct the errors */
      if ( *blue_shoot != *blue_ref )
      {
        FT_Pos   ref      = *blue_ref;
        FT_Pos   shoot    = *blue_shoot;
        FT_Bool  over_ref = FT_BOOL( shoot > ref );


        if ( AH_IS_TOP_BLUE( blue ) ^ over_ref )
          *blue_shoot = *blue_ref = ( shoot + ref ) / 2;
      }

      AH_LOG(( "-- ref = %ld, shoot = %ld\n", *blue_ref, *blue_shoot ));
    }

    /* reset original face charmap */
    FT_Set_Charmap( face, charmap );
    error = 0;

  Exit:
    return error;
  }


  static FT_Error
  ah_hinter_compute_widths( AH_Hinter*  hinter )
  {
    /* scan the array of segments in each direction */
    AH_Outline*  outline = hinter->glyph;
    AH_Segment*  segments;
    AH_Segment*  limit;
    AH_Globals*  globals = &hinter->globals->design;
    FT_Pos*      widths;
    FT_Int       dimension;
    FT_Int*      p_num_widths;
    FT_Error     error = 0;
    FT_Pos       edge_distance_threshold = 32000;


    globals->num_widths  = 0;
    globals->num_heights = 0;

    /* For now, compute the standard width and height from the `o'       */
    /* character.  I started computing the stem width of the `i' and the */
    /* stem height of the "-", but it wasn't too good.  Moreover, we now */
    /* have a single character that gives us standard width and height.  */
    {
      FT_UInt   glyph_index;


      glyph_index = FT_Get_Char_Index( hinter->face, 'o' );
      if ( glyph_index == 0 )
        return 0;

      error = FT_Load_Glyph( hinter->face, glyph_index, FT_LOAD_NO_SCALE );
      if ( error )
        goto Exit;

      error = ah_outline_load( hinter->glyph, hinter->face );
      if ( error )
        goto Exit;

      ah_outline_compute_segments( hinter->glyph );
      ah_outline_link_segments( hinter->glyph );
    }

    segments     = outline->horz_segments;
    limit        = segments + outline->num_hsegments;
    widths       = globals->heights;
    p_num_widths = &globals->num_heights;

    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AH_Segment*  seg = segments;
      AH_Segment*  link;
      FT_Int       num_widths = 0;


      for ( ; seg < limit; seg++ )
      {
        link = seg->link;
        /* we only consider stem segments there! */
        if ( link && link->link == seg && link > seg )
        {
          FT_Int  dist;


          dist = seg->pos - link->pos;
          if ( dist < 0 )
            dist = -dist;

          if ( num_widths < 12 )
            widths[num_widths++] = dist;
        }
      }

      sort_values( num_widths, widths );
      *p_num_widths = num_widths;

      /* we will now try to find the smallest width */
      if ( num_widths > 0 && widths[0] < edge_distance_threshold )
        edge_distance_threshold = widths[0];

      segments     = outline->vert_segments;
      limit        = segments + outline->num_vsegments;
      widths       = globals->widths;
      p_num_widths = &globals->num_widths;

    }

    /* Now, compute the edge distance threshold as a fraction of the */
    /* smallest width in the font. Set it in `hinter.glyph' too!     */
    if ( edge_distance_threshold == 32000 )
      edge_distance_threshold = 50;

    /* let's try 20% */
    hinter->glyph->edge_distance_threshold = edge_distance_threshold / 5;

  Exit:
    return error;
  }


  FT_LOCAL_DEF FT_Error
  ah_hinter_compute_globals( AH_Hinter*  hinter )
  {
    return ah_hinter_compute_widths( hinter ) ||
           ah_hinter_compute_blues ( hinter );
  }


/* END */
