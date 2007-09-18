/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998-2000 Raph Levien
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

#include <string.h>
#include "art_misc.h"
#include "art_uta.h"
#include "art_uta_ops.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/**
 * art_uta_union: Compute union of two uta's.
 * @uta1: One uta.
 * @uta2: The other uta.
 *
 * Computes the union of @uta1 and @uta2. The union is approximate,
 * but coverage is guaranteed over all pixels included in either of
 * the arguments, ie more pixels may be covered than the "exact"
 * union.
 *
 * Note: this routine is used in the Gnome Canvas to accumulate the
 * region that needs to be repainted. However, since it copies over
 * the entire uta (which might be largish) even when the update may be
 * small, it can be a performance bottleneck. There are two approaches
 * to this problem, both of which are probably worthwhile. First, the
 * generated uta's should always be limited to the visible window,
 * thus guaranteeing that uta's never become large. Second, there
 * should be a new, destructive union operation that only touches a
 * small part of the uta when the update is small.
 *
 * Return value: The new union uta.
 **/
ArtUta *
art_uta_union (ArtUta *uta1, ArtUta *uta2)
{
  ArtUta *uta;
  int x0, y0, x1, y1;
  int x, y;
  int ix, ix1, ix2;
  ArtUtaBbox bb, bb1, bb2;

  x0 = MIN(uta1->x0, uta2->x0);
  y0 = MIN(uta1->y0, uta2->y0);
  x1 = MAX(uta1->x0 + uta1->width, uta2->x0 + uta2->width);
  y1 = MAX(uta1->y0 + uta1->height, uta2->y0 + uta2->height);
  uta = art_uta_new (x0, y0, x1, y1);

  /* could move the first two if/else statements out of the loop */
  ix = 0;
  for (y = y0; y < y1; y++)
    {
      ix1 = (y - uta1->y0) * uta1->width + x0 - uta1->x0;
      ix2 = (y - uta2->y0) * uta2->width + x0 - uta2->x0;
      for (x = x0; x < x1; x++)
	{
	  if (x < uta1->x0 || y < uta1->y0 ||
	      x >= uta1->x0 + uta1->width || y >= uta1->y0 + uta1->height)
	    bb1 = 0;
	  else
	    bb1 = uta1->utiles[ix1];

	  if (x < uta2->x0 || y < uta2->y0 ||
	      x >= uta2->x0 + uta2->width || y >= uta2->y0 + uta2->height)
	    bb2 = 0;
	  else
	    bb2 = uta2->utiles[ix2];

	  if (bb1 == 0)
	    bb = bb2;
	  else if (bb2 == 0)
	    bb = bb1;
	  else
	    bb = ART_UTA_BBOX_CONS(MIN(ART_UTA_BBOX_X0(bb1),
				       ART_UTA_BBOX_X0(bb2)),
				   MIN(ART_UTA_BBOX_Y0(bb1),
				       ART_UTA_BBOX_Y0(bb2)),
				   MAX(ART_UTA_BBOX_X1(bb1),
				       ART_UTA_BBOX_X1(bb2)),
				   MAX(ART_UTA_BBOX_Y1(bb1),
				       ART_UTA_BBOX_Y1(bb2)));
	  uta->utiles[ix] = bb;
	  ix++;
	  ix1++;
	  ix2++;
	}
    }
  return uta;
}
