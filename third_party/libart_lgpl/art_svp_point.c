/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1999 Raph Levien
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

#include <math.h>
#include "art_misc.h"

#include "art_svp.h"
#include "art_svp_point.h"

/* Determine whether a point is inside, or near, an svp. */

/* return winding number of point wrt svp */
/**
 * art_svp_point_wind: Determine winding number of a point with respect to svp.
 * @svp: The svp.
 * @x: The X coordinate of the point.
 * @y: The Y coordinate of the point.
 *
 * Determine the winding number of the point @x, @y with respect to @svp.
 *
 * Return value: the winding number.
 **/
int
art_svp_point_wind (ArtSVP *svp, double x, double y)
{
  int i, j;
  int wind = 0;

  for (i = 0; i < svp->n_segs; i++)
    {
      ArtSVPSeg *seg = &svp->segs[i];

      if (seg->bbox.y0 > y)
	break;

      if (seg->bbox.y1 > y)
	{
	  if (seg->bbox.x1 < x)
	    wind += seg->dir ? 1 : -1;
	  else if (seg->bbox.x0 <= x)
	    {
	      double x0, y0, x1, y1, dx, dy;

	      for (j = 0; j < seg->n_points - 1; j++)
		{
		  if (seg->points[j + 1].y > y)
		    break;
		}
	      x0 = seg->points[j].x;
	      y0 = seg->points[j].y;
	      x1 = seg->points[j + 1].x;
	      y1 = seg->points[j + 1].y;

	      dx = x1 - x0;
	      dy = y1 - y0;
	      if ((x - x0) * dy > (y - y0) * dx)
		wind += seg->dir ? 1 : -1;
	    }
	}
    }

  return wind;
}

/**
 * art_svp_point_dist: Determine distance between point and svp.
 * @svp: The svp.
 * @x: The X coordinate of the point.
 * @y: The Y coordinate of the point.
 *
 * Determines the distance of the point @x, @y to the closest edge in
 * @svp. A large number is returned if @svp is empty.
 *
 * Return value: the distance.
 **/
double
art_svp_point_dist (ArtSVP *svp, double x, double y)
{
  int i, j;
  double dist_sq;
  double best_sq = -1;

  for (i = 0; i < svp->n_segs; i++)
    {
      ArtSVPSeg *seg = &svp->segs[i];
      for (j = 0; j < seg->n_points - 1; j++)
	{
	  double x0 = seg->points[j].x;
	  double y0 = seg->points[j].y;
	  double x1 = seg->points[j + 1].x;
	  double y1 = seg->points[j + 1].y;

	  double dx = x1 - x0;
	  double dy = y1 - y0;

	  double dxx0 = x - x0;
	  double dyy0 = y - y0;

	  double dot = dxx0 * dx + dyy0 * dy;

	  if (dot < 0)
	    dist_sq = dxx0 * dxx0 + dyy0 * dyy0;
	  else
	    {
	      double rr = dx * dx + dy * dy;

	      if (dot > rr)
		dist_sq = (x - x1) * (x - x1) + (y - y1) * (y - y1);
	      else
		{
		  double perp = (y - y0) * dx - (x - x0) * dy;

		  dist_sq = perp * perp / rr;
		}
	    }
	  if (best_sq < 0 || dist_sq < best_sq)
	    best_sq = dist_sq;
	}
    }

  if (best_sq >= 0)
    return sqrt (best_sq);
  else
    return 1e12;
}

