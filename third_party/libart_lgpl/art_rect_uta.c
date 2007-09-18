/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "art_misc.h"
#include "art_uta.h"
#include "art_rect.h"
#include "art_rect_uta.h"

/* Functions to decompose a microtile array into a list of rectangles. */

/**
 * art_rect_list_from_uta: Decompose uta into list of rectangles.
 * @uta: The source uta.
 * @max_width: The maximum width of the resulting rectangles.
 * @max_height: The maximum height of the resulting rectangles.
 * @p_nrects: Where to store the number of returned rectangles.
 *
 * Allocates a new list of rectangles, sets *@p_nrects to the number
 * in the list. This list should be freed with art_free().
 *
 * Each rectangle bounded in size by (@max_width, @max_height).
 * However, these bounds must be at least the size of one tile.
 *
 * This routine provides a precise implementation, i.e. the rectangles
 * cover exactly the same area as the uta. It is thus appropriate in
 * cases where the overhead per rectangle is small compared with the
 * cost of filling in extra pixels.
 *
 * Return value: An array containing the resulting rectangles.
 **/
ArtIRect *
art_rect_list_from_uta (ArtUta *uta, int max_width, int max_height,
			int *p_nrects)
{
  ArtIRect *rects;
  int n_rects, n_rects_max;
  int x, y;
  int width, height;
  int ix;
  int left_ix;
  ArtUtaBbox *utiles;
  ArtUtaBbox bb;
  int x0, y0, x1, y1;
  int *glom;
  int glom_rect;

  n_rects = 0;
  n_rects_max = 1;
  rects = art_new (ArtIRect, n_rects_max);

  width = uta->width;
  height = uta->height;
  utiles = uta->utiles;

  glom = art_new (int, width * height);
  for (ix = 0; ix < width * height; ix++)
    glom[ix] = -1;

  ix = 0;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	bb = utiles[ix];
	if (bb)
	  {
	    x0 = ((uta->x0 + x) << ART_UTILE_SHIFT) + ART_UTA_BBOX_X0(bb);
	    y0 = ((uta->y0 + y) << ART_UTILE_SHIFT) + ART_UTA_BBOX_Y0(bb);
	    y1 = ((uta->y0 + y) << ART_UTILE_SHIFT) + ART_UTA_BBOX_Y1(bb);

	    left_ix = ix;
	    /* now try to extend to the right */
	    while (x != width - 1 &&
		   ART_UTA_BBOX_X1(bb) == ART_UTILE_SIZE &&
		   (((bb & 0xffffff) ^ utiles[ix + 1]) & 0xffff00ff) == 0 &&
		   (((uta->x0 + x + 1) << ART_UTILE_SHIFT) +
		    ART_UTA_BBOX_X1(utiles[ix + 1]) -
		    x0) <= max_width)
	      {
		bb = utiles[ix + 1];
		ix++;
		x++;
	      }
	    x1 = ((uta->x0 + x) << ART_UTILE_SHIFT) + ART_UTA_BBOX_X1(bb);


	    /* if rectangle nonempty */
	    if ((x1 ^ x0) | (y1 ^ y0))
	      {
		/* try to glom onto an existing rectangle */
		glom_rect = glom[left_ix];
		if (glom_rect != -1 &&
		    x0 == rects[glom_rect].x0 &&
		    x1 == rects[glom_rect].x1 &&
		    y0 == rects[glom_rect].y1 &&
		    y1 - rects[glom_rect].y0 <= max_height)
		  {
		    rects[glom_rect].y1 = y1;
		  }
		else
		  {
		    if (n_rects == n_rects_max)
		      art_expand (rects, ArtIRect, n_rects_max);
		    rects[n_rects].x0 = x0;
		    rects[n_rects].y0 = y0;
		    rects[n_rects].x1 = x1;
		    rects[n_rects].y1 = y1;
		    glom_rect = n_rects;
		    n_rects++;
		  }
		if (y != height - 1)
		  glom[left_ix + width] = glom_rect;
	      }
	  }
	ix++;
      }

  art_free (glom);
  *p_nrects = n_rects;
  return rects;
}
