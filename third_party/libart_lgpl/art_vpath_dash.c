/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1999-2000 Raph Levien
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

/* Apply a dash style to a vector path. */

#include <math.h>
#include <stdlib.h>

#include "art_misc.h"

#include "art_vpath.h"
#include "art_vpath_dash.h"


/* Return the length of the largest subpath within vpath */
static int
art_vpath_dash_max_subpath (const ArtVpath *vpath)
{
  int max_subpath;
  int i;
  int start;

  max_subpath = 0;
  start = 0;
  for (i = 0; vpath[i].code != ART_END; i++)
    {
      if (vpath[i].code == ART_MOVETO || vpath[i].code == ART_MOVETO_OPEN)
	{
	  if (i - start > max_subpath)
	    max_subpath = i - start;
	  start = i;
	}
    }
  if (i - start > max_subpath)
    max_subpath = i - start;

  return max_subpath;
}

/**
 * art_vpath_dash: Add dash style to vpath.
 * @vpath: Original vpath.
 * @dash: Dash style.
 *
 * Creates a new vpath that is the result of applying dash style @dash
 * to @vpath.
 *
 * This implementation has two known flaws:
 *
 * First, it adds a spurious break at the beginning of the vpath. The
 * only way I see to resolve this flaw is to run the state forward one
 * dash break at the beginning, and fix up by looping back to the
 * first dash break at the end. This is doable but of course adds some
 * complexity.
 *
 * Second, it does not suppress output points that are within epsilon
 * of each other.
 *
 * Return value: Newly created vpath.
 **/
ArtVpath *
art_vpath_dash (const ArtVpath *vpath, const ArtVpathDash *dash)
{
  int max_subpath;
  double *dists;
  ArtVpath *result;
  int n_result, n_result_max;
  int start, end;
  int i;
  double total_dist;

  /* state while traversing dasharray - offset is offset of current dash
     value, toggle is 0 for "off" and 1 for "on", and phase is the distance
     in, >= 0, < dash->dash[offset]. */
  int offset, toggle;
  double phase;

  /* initial values */
  int offset_init, toggle_init;
  double phase_init;

  max_subpath = art_vpath_dash_max_subpath (vpath);
  dists = art_new (double, max_subpath);

  n_result = 0;
  n_result_max = 16;
  result = art_new (ArtVpath, n_result_max);

  /* determine initial values of dash state */
  toggle_init = 1;
  offset_init = 0;
  phase_init = dash->offset;
  while (phase_init >= dash->dash[offset_init])
    {
      toggle_init = !toggle_init;
      phase_init -= dash->dash[offset_init];
      offset_init++;
      if (offset_init == dash->n_dash)
	offset_init = 0;
    }

  for (start = 0; vpath[start].code != ART_END; start = end)
    {
      for (end = start + 1; vpath[end].code == ART_LINETO; end++);
      /* subpath is [start..end) */
      total_dist = 0;
      for (i = start; i < end - 1; i++)
	{
	  double dx, dy;

	  dx = vpath[i + 1].x - vpath[i].x;
	  dy = vpath[i + 1].y - vpath[i].y;
	  dists[i - start] = sqrt (dx * dx + dy * dy);
	  total_dist += dists[i - start];
	}
      if (total_dist <= dash->dash[offset_init] - phase_init)
	{
	  /* subpath fits entirely within first dash */
	  if (toggle_init)
	    {
	      for (i = start; i < end; i++)
		art_vpath_add_point (&result, &n_result, &n_result_max,
				     vpath[i].code, vpath[i].x, vpath[i].y);
	    }
	}
      else
	{
	  /* subpath is composed of at least one dash - thus all
	     generated pieces are open */
	  double dist;

	  phase = phase_init;
	  offset = offset_init;
	  toggle = toggle_init;
	  dist = 0;
	  i = start;
	  if (toggle)
	    art_vpath_add_point (&result, &n_result, &n_result_max,
				 ART_MOVETO_OPEN, vpath[i].x, vpath[i].y);
	  while (i != end - 1)
	    {
	      if (dists[i - start] - dist > dash->dash[offset] - phase)
		{
		  /* dash boundary is next */
		  double a;
		  double x, y;

		  dist += dash->dash[offset] - phase;
		  a = dist / dists[i - start];
		  x = vpath[i].x + a * (vpath[i + 1].x - vpath[i].x);
		  y = vpath[i].y + a * (vpath[i + 1].y - vpath[i].y);
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				       toggle ? ART_LINETO : ART_MOVETO_OPEN,
				       x, y);
		  /* advance to next dash */
		  toggle = !toggle;
		  phase = 0;
		  offset++;
		  if (offset == dash->n_dash)
		    offset = 0;
		}
	      else
		{
		  /* end of line in vpath is next */
		  phase += dists[i - start] - dist;
		  i++;
		  dist = 0;
		  if (toggle)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
					 ART_LINETO, vpath[i].x, vpath[i].y);
		}
	    }
	}
    }

  art_vpath_add_point (&result, &n_result, &n_result_max,
		       ART_END, 0, 0);

  art_free (dists);

  return result;
}
