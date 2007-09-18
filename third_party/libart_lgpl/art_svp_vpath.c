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

/* Sort vector paths into sorted vector paths */

#include <stdlib.h>
#include <math.h>

#include "art_misc.h"

#include "art_vpath.h"
#include "art_svp.h"
#include "art_svp_vpath.h"


/* reverse a list of points in place */
static void
reverse_points (ArtPoint *points, int n_points)
{
  int i;
  ArtPoint tmp_p;

  for (i = 0; i < (n_points >> 1); i++)
    {
      tmp_p = points[i];
      points[i] = points[n_points - (i + 1)];
      points[n_points - (i + 1)] = tmp_p;
    }
}

/**
 * art_svp_from_vpath: Convert a vpath to a sorted vector path.
 * @vpath: #ArtVPath to convert.
 *
 * Converts a vector path into sorted vector path form. The svp form is
 * more efficient for rendering and other vector operations.
 *
 * Basically, the implementation is to traverse the vector path,
 * generating a new segment for each "run" of points in the vector
 * path with monotonically increasing Y values. All the resulting
 * values are then sorted.
 *
 * Note: I'm not sure that the sorting rule is correct with respect
 * to numerical stability issues.
 *
 * Return value: Resulting sorted vector path.
 **/
ArtSVP *
art_svp_from_vpath (ArtVpath *vpath)
{
  int n_segs, n_segs_max;
  ArtSVP *svp;
  int dir;
  int new_dir;
  int i;
  ArtPoint *points;
  int n_points, n_points_max;
  double x, y;
  double x_min, x_max;

  n_segs = 0;
  n_segs_max = 16;
  svp = (ArtSVP *)art_alloc (sizeof(ArtSVP) +
			     (n_segs_max - 1) * sizeof(ArtSVPSeg));

  dir = 0;
  n_points = 0;
  n_points_max = 0;
  points = NULL;
  i = 0;

  x = y = 0; /* unnecessary, given "first code must not be LINETO" invariant,
		but it makes gcc -Wall -ansi -pedantic happier */
  x_min = x_max = 0; /* same */

  while (vpath[i].code != ART_END) {
    if (vpath[i].code == ART_MOVETO || vpath[i].code == ART_MOVETO_OPEN)
      {
	if (points != NULL && n_points >= 2)
	  {
	    if (n_segs == n_segs_max)
	      {
		n_segs_max <<= 1;
		svp = (ArtSVP *)art_realloc (svp, sizeof(ArtSVP) +
					     (n_segs_max - 1) *
					     sizeof(ArtSVPSeg));
	      }
	    svp->segs[n_segs].n_points = n_points;
	    svp->segs[n_segs].dir = (dir > 0);
	    if (dir < 0)
	      reverse_points (points, n_points);
	    svp->segs[n_segs].points = points;
	    svp->segs[n_segs].bbox.x0 = x_min;
	    svp->segs[n_segs].bbox.x1 = x_max;
	    svp->segs[n_segs].bbox.y0 = points[0].y;
	    svp->segs[n_segs].bbox.y1 = points[n_points - 1].y;
	    n_segs++;
	    points = NULL;
	  }

	if (points == NULL)
	  {
	    n_points_max = 4;
	    points = art_new (ArtPoint, n_points_max);
	  }

	n_points = 1;
	points[0].x = x = vpath[i].x;
	points[0].y = y = vpath[i].y;
	x_min = x;
	x_max = x;
	dir = 0;
      }
    else /* must be LINETO */
      {
	new_dir = (vpath[i].y > y ||
		   (vpath[i].y == y && vpath[i].x > x)) ? 1 : -1;
	if (dir && dir != new_dir)
	  {
	    /* new segment */
	    x = points[n_points - 1].x;
	    y = points[n_points - 1].y;
	    if (n_segs == n_segs_max)
	      {
		n_segs_max <<= 1;
		svp = (ArtSVP *)art_realloc (svp, sizeof(ArtSVP) +
					     (n_segs_max - 1) *
					     sizeof(ArtSVPSeg));
	      }
	    svp->segs[n_segs].n_points = n_points;
	    svp->segs[n_segs].dir = (dir > 0);
	    if (dir < 0)
	      reverse_points (points, n_points);
	    svp->segs[n_segs].points = points;
	    svp->segs[n_segs].bbox.x0 = x_min;
	    svp->segs[n_segs].bbox.x1 = x_max;
	    svp->segs[n_segs].bbox.y0 = points[0].y;
	    svp->segs[n_segs].bbox.y1 = points[n_points - 1].y;
	    n_segs++;

	    n_points = 1;
	    n_points_max = 4;
	    points = art_new (ArtPoint, n_points_max);
	    points[0].x = x;
	    points[0].y = y;
	    x_min = x;
	    x_max = x;
	  }

	if (points != NULL)
	  {
	    if (n_points == n_points_max)
	      art_expand (points, ArtPoint, n_points_max);
	    points[n_points].x = x = vpath[i].x;
	    points[n_points].y = y = vpath[i].y;
	    if (x < x_min) x_min = x;
	    else if (x > x_max) x_max = x;
	    n_points++;
	  }
	dir = new_dir;
      }
    i++;
  }

  if (points != NULL)
    {
      if (n_points >= 2)
	{
	  if (n_segs == n_segs_max)
	    {
	      n_segs_max <<= 1;
	      svp = (ArtSVP *)art_realloc (svp, sizeof(ArtSVP) +
					   (n_segs_max - 1) *
					   sizeof(ArtSVPSeg));
	    }
	  svp->segs[n_segs].n_points = n_points;
	  svp->segs[n_segs].dir = (dir > 0);
	  if (dir < 0)
	    reverse_points (points, n_points);
	  svp->segs[n_segs].points = points;
	  svp->segs[n_segs].bbox.x0 = x_min;
	  svp->segs[n_segs].bbox.x1 = x_max;
	  svp->segs[n_segs].bbox.y0 = points[0].y;
	  svp->segs[n_segs].bbox.y1 = points[n_points - 1].y;
	  n_segs++;
	}
      else
	art_free (points);
    }

  svp->n_segs = n_segs;

  qsort (&svp->segs, n_segs, sizeof (ArtSVPSeg), art_svp_seg_compare);

  return svp;
}

