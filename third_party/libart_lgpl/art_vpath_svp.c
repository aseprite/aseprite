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

/* "Unsort" a sorted vector path into an ordinary vector path. */

#include <stdio.h> /* for printf - debugging */
#include "art_misc.h"

#include "art_vpath.h"
#include "art_svp.h"
#include "art_vpath_svp.h"

typedef struct _ArtVpathSVPEnd ArtVpathSVPEnd;

struct _ArtVpathSVPEnd {
  int seg_num;
  int which; /* 0 = top, 1 = bottom */
  double x, y;
};

#define EPSILON 1e-6

static int
art_vpath_svp_point_compare (double x1, double y1, double x2, double y2)
{
  if (y1 - EPSILON > y2) return 1;
  if (y1 + EPSILON < y2) return -1;
  if (x1 - EPSILON > x2) return 1;
  if (x1 + EPSILON < x2) return -1;
  return 0;
}

static int
art_vpath_svp_compare (const void *s1, const void *s2)
{
  const ArtVpathSVPEnd *e1 = s1;
  const ArtVpathSVPEnd *e2 = s2;

  return art_vpath_svp_point_compare (e1->x, e1->y, e2->x, e2->y);
}

/* Convert from sorted vector path representation into regular
   vector path representation.

   Status of this routine:

   Basic correctness: Only works with closed paths.

   Numerical stability: Not known to work when more than two segments
   meet at a point.

   Speed: Should be pretty good.

   Precision: Does not degrade precision.

*/
/**
 * art_vpath_from_svp: Convert from svp to vpath form.
 * @svp: Original #ArtSVP.
 *
 * Converts the sorted vector path @svp into standard vpath form.
 *
 * Return value: the newly allocated vpath.
 **/
ArtVpath *
art_vpath_from_svp (const ArtSVP *svp)
{
  int n_segs = svp->n_segs;
  ArtVpathSVPEnd *ends;
  ArtVpath *new;
  int *visited;
  int n_new, n_new_max;
  int i, j, k;
  int seg_num;
  int first;
  double last_x, last_y;
  int n_points;
  int pt_num;

  last_x = 0; /* to eliminate "uninitialized" warning */
  last_y = 0;

  ends = art_new (ArtVpathSVPEnd, n_segs * 2);
  for (i = 0; i < svp->n_segs; i++)
    {
      int lastpt;

      ends[i * 2].seg_num = i;
      ends[i * 2].which = 0;
      ends[i * 2].x = svp->segs[i].points[0].x;
      ends[i * 2].y = svp->segs[i].points[0].y;

      lastpt = svp->segs[i].n_points - 1;
      ends[i * 2 + 1].seg_num = i;
      ends[i * 2 + 1].which = 1;
      ends[i * 2 + 1].x = svp->segs[i].points[lastpt].x;
      ends[i * 2 + 1].y = svp->segs[i].points[lastpt].y;
    }
  qsort (ends, n_segs * 2, sizeof (ArtVpathSVPEnd), art_vpath_svp_compare);

  n_new = 0;
  n_new_max = 16; /* I suppose we _could_ estimate this from traversing
		     the svp, so we don't have to reallocate */
  new = art_new (ArtVpath, n_new_max);

  visited = art_new (int, n_segs);
  for (i = 0; i < n_segs; i++)
    visited[i] = 0;

  first = 1;
  for (i = 0; i < n_segs; i++)
    {
      if (!first)
	{
	  /* search for the continuation of the existing subpath */
	  /* This could be a binary search (which is why we sorted, above) */
	  for (j = 0; j < n_segs * 2; j++)
	    {
	      if (!visited[ends[j].seg_num] &&
		  art_vpath_svp_point_compare (last_x, last_y,
					       ends[j].x, ends[j].y) == 0)
		break;
	    }
	  if (j == n_segs * 2)
	    first = 1;
	}
      if (first)
	{
	  /* start a new subpath */
	  for (j = 0; j < n_segs * 2; j++)
	    if (!visited[ends[j].seg_num])
	      break;
	}
      if (j == n_segs * 2)
	{
	  printf ("failure\n");
	}
      seg_num = ends[j].seg_num;
      n_points = svp->segs[seg_num].n_points;
      for (k = 0; k < n_points; k++)
	{
	  pt_num = svp->segs[seg_num].dir ? k : n_points - (1 + k);
	  if (k == 0)
	    {
	      if (first)
		{
		  art_vpath_add_point (&new, &n_new, &n_new_max,
				       ART_MOVETO,
				       svp->segs[seg_num].points[pt_num].x,
				       svp->segs[seg_num].points[pt_num].y);
		}
	    }
	  else
	    {
	      art_vpath_add_point (&new, &n_new, &n_new_max,
				   ART_LINETO,
				   svp->segs[seg_num].points[pt_num].x,
				   svp->segs[seg_num].points[pt_num].y);
	      if (k == n_points - 1)
		{
		  last_x = svp->segs[seg_num].points[pt_num].x;
		  last_y = svp->segs[seg_num].points[pt_num].y;
		  /* to make more robust, check for meeting first_[xy],
		     set first if so */
		}
	    }
	  first = 0;
	}
      visited[seg_num] = 1;
    }

  art_vpath_add_point (&new, &n_new, &n_new_max,
		       ART_END, 0, 0);
  art_free (visited);
  art_free (ends);
  return new;
}
