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

/* Primitive intersection and winding number operations on sorted
   vector paths.

   These routines are internal to libart, used to construct operations
   like intersection, union, and difference. */


#include <stdio.h> /* for printf of debugging info */
#include <string.h> /* for memcpy */
#include <math.h>
#include "art_misc.h"

#include "art_rect.h"
#include "art_svp.h"
#include "art_svp_wind.h"

#define noVERBOSE

#define PT_EQ(p1,p2) ((p1).x == (p2).x && (p1).y == (p2).y)

#define PT_CLOSE(p1,p2) (fabs ((p1).x - (p2).x) < 1e-6 && fabs ((p1).y - (p2).y) < 1e-6)

/* return nonzero and set *p to the intersection point if the lines
   z0-z1 and z2-z3 intersect each other. */
static int
intersect_lines (ArtPoint z0, ArtPoint z1, ArtPoint z2, ArtPoint z3,
		 ArtPoint *p)
{
  double a01, b01, c01;
  double a23, b23, c23;
  double d0, d1, d2, d3;
  double det;

  /* if the vectors share an endpoint, they don't intersect */
  if (PT_EQ (z0, z2) || PT_EQ (z0, z3) || PT_EQ (z1, z2) || PT_EQ (z1, z3))
    return 0;

#if 0
  if (PT_CLOSE (z0, z2) || PT_CLOSE (z0, z3) || PT_CLOSE (z1, z2) || PT_CLOSE (z1, z3))
    return 0;
#endif

  /* find line equations ax + by + c = 0 */
  a01 = z0.y - z1.y;
  b01 = z1.x - z0.x;
  c01 = -(z0.x * a01 + z0.y * b01);
  /* = -((z0.y - z1.y) * z0.x + (z1.x - z0.x) * z0.y) 
     = (z1.x * z0.y - z1.y * z0.x) */

  d2 = a01 * z2.x + b01 * z2.y + c01;
  d3 = a01 * z3.x + b01 * z3.y + c01;
  if ((d2 > 0) == (d3 > 0))
    return 0;

  a23 = z2.y - z3.y;
  b23 = z3.x - z2.x;
  c23 = -(z2.x * a23 + z2.y * b23);

  d0 = a23 * z0.x + b23 * z0.y + c23;
  d1 = a23 * z1.x + b23 * z1.y + c23;
  if ((d0 > 0) == (d1 > 0))
    return 0;

  /* now we definitely know that the lines intersect */
  /* solve the two linear equations ax + by + c = 0 */
  det = 1.0 / (a01 * b23 - a23 * b01);
  p->x = det * (c23 * b01 - c01 * b23);
  p->y = det * (c01 * a23 - c23 * a01);

  return 1;
}

#define EPSILON 1e-6

static double
trap_epsilon (double v)
{
  const double epsilon = EPSILON;

  if (v < epsilon && v > -epsilon) return 0;
  else return v;
}

/* Determine the order of line segments z0-z1 and z2-z3.
   Return +1 if z2-z3 lies entirely to the right of z0-z1,
   -1 if entirely to the left,
   or 0 if overlap.

   The case analysis in this function is quite ugly. The fact that it's
   almost 200 lines long is ridiculous.

   Ok, so here's the plan to cut it down:

   First, do a bounding line comparison on the x coordinates. This is pretty
   much the common case, and should go quickly. It also takes care of the
   case where both lines are horizontal.

   Then, do d0 and d1 computation, but only if a23 is nonzero.

   Finally, do d2 and d3 computation, but only if a01 is nonzero.

   Fall through to returning 0 (this will happen when both lines are
   horizontal and they overlap).
   */
static int
x_order (ArtPoint z0, ArtPoint z1, ArtPoint z2, ArtPoint z3)
{
  double a01, b01, c01;
  double a23, b23, c23;
  double d0, d1, d2, d3;

  if (z0.y == z1.y)
    {
      if (z2.y == z3.y)
	{
	  double x01min, x01max;
	  double x23min, x23max;

	  if (z0.x > z1.x)
	    {
	      x01min = z1.x;
	      x01max = z0.x;
	    }
	  else
	    {
	      x01min = z0.x;
	      x01max = z1.x;
	    }

	  if (z2.x > z3.x)
	    {
	      x23min = z3.x;
	      x23max = z2.x;
	    }
	  else
	    {
	      x23min = z2.x;
	      x23max = z3.x;
	    }

	  if (x23min >= x01max) return 1;
	  else if (x01min >= x23max) return -1;
	  else return 0;
	}
      else
	{
	  /* z0-z1 is horizontal, z2-z3 isn't */
	  a23 = z2.y - z3.y;
	  b23 = z3.x - z2.x;
	  c23 = -(z2.x * a23 + z2.y * b23);

	  if (z3.y < z2.y)
	    {
	      a23 = -a23;
	      b23 = -b23;
	      c23 = -c23;
	    }
	  
	  d0 = trap_epsilon (a23 * z0.x + b23 * z0.y + c23);
	  d1 = trap_epsilon (a23 * z1.x + b23 * z1.y + c23);

	  if (d0 > 0)
	    {
	      if (d1 >= 0) return 1;
	      else return 0;
	    }
	  else if (d0 == 0)
	    {
	      if (d1 > 0) return 1;
	      else if (d1 < 0) return -1;
	      else printf ("case 1 degenerate\n");
	      return 0;
	    }
	  else /* d0 < 0 */
	    {
	      if (d1 <= 0) return -1;
	      else return 0;
	    }
	}
    }
  else if (z2.y == z3.y)
    {
      /* z2-z3 is horizontal, z0-z1 isn't */
      a01 = z0.y - z1.y;
      b01 = z1.x - z0.x;
      c01 = -(z0.x * a01 + z0.y * b01);
      /* = -((z0.y - z1.y) * z0.x + (z1.x - z0.x) * z0.y) 
	 = (z1.x * z0.y - z1.y * z0.x) */

      if (z1.y < z0.y)
	{
	  a01 = -a01;
	  b01 = -b01;
	  c01 = -c01;
	}

      d2 = trap_epsilon (a01 * z2.x + b01 * z2.y + c01);
      d3 = trap_epsilon (a01 * z3.x + b01 * z3.y + c01);

      if (d2 > 0)
	{
	  if (d3 >= 0) return -1;
	  else return 0;
	}
      else if (d2 == 0)
	{
	  if (d3 > 0) return -1;
	  else if (d3 < 0) return 1;
	  else printf ("case 2 degenerate\n");
	  return 0;
	}
      else /* d2 < 0 */
	{
	  if (d3 <= 0) return 1;
	  else return 0;
	}
    }

  /* find line equations ax + by + c = 0 */
  a01 = z0.y - z1.y;
  b01 = z1.x - z0.x;
  c01 = -(z0.x * a01 + z0.y * b01);
  /* = -((z0.y - z1.y) * z0.x + (z1.x - z0.x) * z0.y) 
     = -(z1.x * z0.y - z1.y * z0.x) */

  if (a01 > 0)
    {
      a01 = -a01;
      b01 = -b01;
      c01 = -c01;
    }
  /* so now, (a01, b01) points to the left, thus a01 * x + b01 * y + c01
     is negative if the point lies to the right of the line */

  d2 = trap_epsilon (a01 * z2.x + b01 * z2.y + c01);
  d3 = trap_epsilon (a01 * z3.x + b01 * z3.y + c01);
  if (d2 > 0)
    {
      if (d3 >= 0) return -1;
    }
  else if (d2 == 0)
    {
      if (d3 > 0) return -1;
      else if (d3 < 0) return 1;
      else
	fprintf (stderr, "colinear!\n");
    }
  else /* d2 < 0 */
    {
      if (d3 <= 0) return 1;
    }

  a23 = z2.y - z3.y;
  b23 = z3.x - z2.x;
  c23 = -(z2.x * a23 + z2.y * b23);

  if (a23 > 0)
    {
      a23 = -a23;
      b23 = -b23;
      c23 = -c23;
    }
  d0 = trap_epsilon (a23 * z0.x + b23 * z0.y + c23);
  d1 = trap_epsilon (a23 * z1.x + b23 * z1.y + c23);
  if (d0 > 0)
    {
      if (d1 >= 0) return 1;
    }
  else if (d0 == 0)
    {
      if (d1 > 0) return 1;
      else if (d1 < 0) return -1;
      else
	fprintf (stderr, "colinear!\n");
    }
  else /* d0 < 0 */
    {
      if (d1 <= 0) return -1;
    }

  return 0;
}

/* similar to x_order, but to determine whether point z0 + epsilon lies to
   the left of the line z2-z3 or to the right */
static int
x_order_2 (ArtPoint z0, ArtPoint z1, ArtPoint z2, ArtPoint z3)
{
  double a23, b23, c23;
  double d0, d1;

  a23 = z2.y - z3.y;
  b23 = z3.x - z2.x;
  c23 = -(z2.x * a23 + z2.y * b23);

  if (a23 > 0)
    {
      a23 = -a23;
      b23 = -b23;
      c23 = -c23;
    }

  d0 = a23 * z0.x + b23 * z0.y + c23;

  if (d0 > EPSILON)
    return -1;
  else if (d0 < -EPSILON)
    return 1;

  d1 = a23 * z1.x + b23 * z1.y + c23;
  if (d1 > EPSILON)
    return -1;
  else if (d1 < -EPSILON)
    return 1;

  if (z0.x <= z2.x && z1.x <= z2.x && z0.x <= z3.x && z1.x <= z3.x)
    return -1;
  if (z0.x >= z2.x && z1.x >= z2.x && z0.x >= z3.x && z1.x >= z3.x)
    return 1;
  
  fprintf (stderr, "x_order_2: colinear!\n");
  return 0;
}

#ifdef DEAD_CODE
/* Traverse the vector path, keeping it in x-sorted order.

   This routine doesn't actually do anything - it's just here for
   explanatory purposes. */
void
traverse (ArtSVP *vp)
{
  int *active_segs;
  int n_active_segs;
  int *cursor;
  int seg_idx;
  double y;
  int tmp1, tmp2;
  int asi;
  int i, j;

  active_segs = art_new (int, vp->n_segs);
  cursor = art_new (int, vp->n_segs);

  n_active_segs = 0;
  seg_idx = 0;
  y = vp->segs[0].points[0].y;
  while (seg_idx < vp->n_segs || n_active_segs > 0)
    {
      printf ("y = %g\n", y);
      /* delete segments ending at y from active list */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  if (vp->segs[asi].n_points - 1 == cursor[asi] &&
	      vp->segs[asi].points[cursor[asi]].y == y)
	    {
	      printf ("deleting %d\n", asi);
	      n_active_segs--;
	      for (j = i; j < n_active_segs; j++)
		active_segs[j] = active_segs[j + 1];
	      i--;
	    }
	}

      /* insert new segments into the active list */
      while (seg_idx < vp->n_segs && y == vp->segs[seg_idx].points[0].y)
	{
	  cursor[seg_idx] = 0;
	  printf ("inserting %d\n", seg_idx);
	  for (i = 0; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (x_order (vp->segs[asi].points[cursor[asi]],
			   vp->segs[asi].points[cursor[asi] + 1],
			   vp->segs[seg_idx].points[0],
			   vp->segs[seg_idx].points[1]) == -1)
	      break;
	    }
	  tmp1 = seg_idx;
	  for (j = i; j < n_active_segs; j++)
	    {
	      tmp2 = active_segs[j];
	      active_segs[j] = tmp1;
	      tmp1 = tmp2;
	    }
	  active_segs[n_active_segs] = tmp1;
	  n_active_segs++;
	  seg_idx++;
	}

      /* all active segs cross the y scanline (considering segs to be
       closed on top and open on bottom) */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  printf ("%d (%g, %g) - (%g, %g) %s\n", asi,
		  vp->segs[asi].points[cursor[asi]].x,
		  vp->segs[asi].points[cursor[asi]].y,
		  vp->segs[asi].points[cursor[asi] + 1].x,
		  vp->segs[asi].points[cursor[asi] + 1].y,
		  vp->segs[asi].dir ? "v" : "^");
	}

      /* advance y to the next event */
      if (n_active_segs == 0)
	{
	  if (seg_idx < vp->n_segs)
	    y = vp->segs[seg_idx].points[0].y;
	  /* else we're done */
	}
      else
	{
	  asi = active_segs[0];
	  y = vp->segs[asi].points[cursor[asi] + 1].y;
	  for (i = 1; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (y > vp->segs[asi].points[cursor[asi] + 1].y)
		y = vp->segs[asi].points[cursor[asi] + 1].y;
	    }
	  if (seg_idx < vp->n_segs && y > vp->segs[seg_idx].points[0].y)
	    y = vp->segs[seg_idx].points[0].y;
	}

      /* advance cursors to reach new y */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  while (cursor[asi] < vp->segs[asi].n_points - 1 &&
		 y >= vp->segs[asi].points[cursor[asi] + 1].y)
	    cursor[asi]++;
	}
      printf ("\n");
    }
  art_free (cursor);
  art_free (active_segs);
}
#endif

/* I believe that the loop will always break with i=1.

   I think I'll want to change this from a simple sorted list to a
   modified stack. ips[*][0] will get its own data structure, and
   ips[*] will in general only be allocated if there is an intersection.
   Finally, the segment can be traced through the initial point
   (formerly ips[*][0]), backwards through the stack, and finally
   to cursor + 1.

   This change should cut down on allocation bandwidth, and also
   eliminate the iteration through n_ipl below.

*/
static void
insert_ip (int seg_i, int *n_ips, int *n_ips_max, ArtPoint **ips, ArtPoint ip)
{
  int i;
  ArtPoint tmp1, tmp2;
  int n_ipl;
  ArtPoint *ipl;

  n_ipl = n_ips[seg_i]++;
  if (n_ipl == n_ips_max[seg_i])
      art_expand (ips[seg_i], ArtPoint, n_ips_max[seg_i]);
  ipl = ips[seg_i];
  for (i = 1; i < n_ipl; i++)
    if (ipl[i].y > ip.y)
      break;
  tmp1 = ip;
  for (; i <= n_ipl; i++)
    {
      tmp2 = ipl[i];
      ipl[i] = tmp1;
      tmp1 = tmp2;
    }
}

/* test active segment (i - 1) against i for intersection, if
   so, add intersection point to both ips lists. */
static void
intersect_neighbors (int i, int *active_segs,
		     int *n_ips, int *n_ips_max, ArtPoint **ips,
		     int *cursor, ArtSVP *vp)
{
  ArtPoint z0, z1, z2, z3;
  int asi01, asi23;
  ArtPoint ip;

  asi01 = active_segs[i - 1];

  z0 = ips[asi01][0];
  if (n_ips[asi01] == 1)
    z1 = vp->segs[asi01].points[cursor[asi01] + 1];
  else
    z1 = ips[asi01][1];

  asi23 = active_segs[i];

  z2 = ips[asi23][0];
  if (n_ips[asi23] == 1)
    z3 = vp->segs[asi23].points[cursor[asi23] + 1];
  else
    z3 = ips[asi23][1];

  if (intersect_lines (z0, z1, z2, z3, &ip))
    {
#ifdef VERBOSE
      printf ("new intersection point: (%g, %g)\n", ip.x, ip.y);
#endif
      insert_ip (asi01, n_ips, n_ips_max, ips, ip);
      insert_ip (asi23, n_ips, n_ips_max, ips, ip);
    }
}

/* Add a new point to a segment in the svp.

   Here, we also check to make sure that the segments satisfy nocross.
   However, this is only valuable for debugging, and could possibly be
   removed.
*/
static void
svp_add_point (ArtSVP *svp, int *n_points_max,
	       ArtPoint p, int *seg_map, int *active_segs, int n_active_segs,
	       int i)
{
  int asi, asi_left, asi_right;
  int n_points, n_points_left, n_points_right;
  ArtSVPSeg *seg;

  asi = seg_map[active_segs[i]];
  seg = &svp->segs[asi];
  n_points = seg->n_points;
  /* find out whether neighboring segments share a point */
  if (i > 0)
    {
      asi_left = seg_map[active_segs[i - 1]];
      n_points_left = svp->segs[asi_left].n_points;
      if (n_points_left > 1 && 
	  PT_EQ (svp->segs[asi_left].points[n_points_left - 2],
		 svp->segs[asi].points[n_points - 1]))
	{
	  /* ok, new vector shares a top point with segment to the left -
	     now, check that it satisfies ordering invariant */
	  if (x_order (svp->segs[asi_left].points[n_points_left - 2],
		       svp->segs[asi_left].points[n_points_left - 1],
		       svp->segs[asi].points[n_points - 1],
		       p) < 1)

	    {
#ifdef VERBOSE
	      printf ("svp_add_point: cross on left!\n");
#endif
	    }
	}
    }

  if (i + 1 < n_active_segs)
    {
      asi_right = seg_map[active_segs[i + 1]];
      n_points_right = svp->segs[asi_right].n_points;
      if (n_points_right > 1 && 
	  PT_EQ (svp->segs[asi_right].points[n_points_right - 2],
		 svp->segs[asi].points[n_points - 1]))
	{
	  /* ok, new vector shares a top point with segment to the right -
	     now, check that it satisfies ordering invariant */
	  if (x_order (svp->segs[asi_right].points[n_points_right - 2],
		       svp->segs[asi_right].points[n_points_right - 1],
		       svp->segs[asi].points[n_points - 1],
		       p) > -1)
	    {
#ifdef VERBOSE
	      printf ("svp_add_point: cross on right!\n");
#endif
	    }
	}
    }
  if (n_points_max[asi] == n_points)
    art_expand (seg->points, ArtPoint, n_points_max[asi]);
  seg->points[n_points] = p;
  if (p.x < seg->bbox.x0)
    seg->bbox.x0 = p.x;
  else if (p.x > seg->bbox.x1)
    seg->bbox.x1 = p.x;
  seg->bbox.y1 = p.y;
  seg->n_points++;
}

#if 0
/* find where the segment (currently at i) is supposed to go, and return
   the target index - if equal to i, then there is no crossing problem.

   "Where it is supposed to go" is defined as following:

   Delete element i, re-insert at position target (bumping everything
   target and greater to the right).
   */
static int
find_crossing (int i, int *active_segs, int n_active_segs,
	       int *cursor, ArtPoint **ips, int *n_ips, ArtSVP *vp)
{
  int asi, asi_left, asi_right;
  ArtPoint p0, p1;
  ArtPoint p0l, p1l;
  ArtPoint p0r, p1r;
  int target;

  asi = active_segs[i];
  p0 = ips[asi][0];
  if (n_ips[asi] == 1)
    p1 = vp->segs[asi].points[cursor[asi] + 1];
  else
    p1 = ips[asi][1];

  for (target = i; target > 0; target--)
    {
      asi_left = active_segs[target - 1];
      p0l = ips[asi_left][0];
      if (n_ips[asi_left] == 1)
	p1l = vp->segs[asi_left].points[cursor[asi_left] + 1];
      else
	p1l = ips[asi_left][1];
      if (!PT_EQ (p0, p0l))
	break;

#ifdef VERBOSE
      printf ("point matches on left (%g, %g) - (%g, %g) x (%g, %g) - (%g, %g)!\n",
	      p0l.x, p0l.y, p1l.x, p1l.y, p0.x, p0.y, p1.x, p1.y);
#endif
      if (x_order (p0l, p1l, p0, p1) == 1)
	break;

#ifdef VERBOSE
      printf ("scanning to the left (i=%d, target=%d)\n", i, target);
#endif
    }

  if (target < i) return target;

  for (; target < n_active_segs - 1; target++)
    {
      asi_right = active_segs[target + 1];
      p0r = ips[asi_right][0];
      if (n_ips[asi_right] == 1)
	p1r = vp->segs[asi_right].points[cursor[asi_right] + 1];
      else
	p1r = ips[asi_right][1];
      if (!PT_EQ (p0, p0r))
	break;

#ifdef VERBOSE
      printf ("point matches on left (%g, %g) - (%g, %g) x (%g, %g) - (%g, %g)!\n",
	      p0.x, p0.y, p1.x, p1.y, p0r.x, p0r.y, p1r.x, p1r.y);
#endif
      if (x_order (p0r, p1r, p0, p1) == 1)
	break;

#ifdef VERBOSE
      printf ("scanning to the right (i=%d, target=%d)\n", i, target);
#endif
    }

  return target;
}
#endif

/* This routine handles the case where the segment changes its position
   in the active segment list. Generally, this will happen when the
   segment (defined by i and cursor) shares a top point with a neighbor,
   but breaks the ordering invariant.

   Essentially, this routine sorts the lines [start..end), all of which
   share a top point. This is implemented as your basic insertion sort.

   This routine takes care of intersecting the appropriate neighbors,
   as well.

   A first argument of -1 immediately returns, which helps reduce special
   casing in the main unwind routine.
*/
static void
fix_crossing (int start, int end, int *active_segs, int n_active_segs,
	      int *cursor, ArtPoint **ips, int *n_ips, int *n_ips_max,
	      ArtSVP *vp, int *seg_map,
	      ArtSVP **p_new_vp, int *pn_segs_max,
	      int **pn_points_max)
{
  int i, j;
  int target;
  int asi, asj;
  ArtPoint p0i, p1i;
  ArtPoint p0j, p1j;
  int swap = 0;
#ifdef VERBOSE
  int k;
#endif
  ArtPoint *pts;

#ifdef VERBOSE
  printf ("fix_crossing: [%d..%d)", start, end);
  for (k = 0; k < n_active_segs; k++)
    printf (" %d", active_segs[k]);
  printf ("\n");
#endif

  if (start == -1)
    return;

  for (i = start + 1; i < end; i++)
    {

      asi = active_segs[i];
      if (cursor[asi] < vp->segs[asi].n_points - 1) {
	p0i = ips[asi][0];
	if (n_ips[asi] == 1)
	  p1i = vp->segs[asi].points[cursor[asi] + 1];
	else
	  p1i = ips[asi][1];

	for (j = i - 1; j >= start; j--)
	  {
	    asj = active_segs[j];
	    if (cursor[asj] < vp->segs[asj].n_points - 1)
	      {
		p0j = ips[asj][0];
		if (n_ips[asj] == 1)
		  p1j = vp->segs[asj].points[cursor[asj] + 1];
		else
		  p1j = ips[asj][1];

		/* we _hope_ p0i = p0j */
		if (x_order_2 (p0j, p1j, p0i, p1i) == -1)
		  break;
	      }
	  }

	target = j + 1;
	/* target is where active_seg[i] _should_ be in active_segs */
      
	if (target != i)
	  {
	    swap = 1;

#ifdef VERBOSE
	    printf ("fix_crossing: at %i should be %i\n", i, target);
#endif

	    /* let's close off all relevant segments */
	    for (j = i; j >= target; j--)
	      {
		asi = active_segs[j];
		/* First conjunct: this isn't the last point in the original
		   segment.

		   Second conjunct: this isn't the first point in the new
		   segment (i.e. already broken).
		*/
		if (cursor[asi] < vp->segs[asi].n_points - 1 &&
		    (*p_new_vp)->segs[seg_map[asi]].n_points != 1)
		  {
		    int seg_num;
		    /* so break here */
#ifdef VERBOSE
		    printf ("closing off %d\n", j);
#endif

		    pts = art_new (ArtPoint, 16);
		    pts[0] = ips[asi][0];
		    seg_num = art_svp_add_segment (p_new_vp, pn_segs_max,
						   pn_points_max,
						   1, vp->segs[asi].dir,
						   pts,
						   NULL);
		    (*pn_points_max)[seg_num] = 16;
		    seg_map[asi] = seg_num;
		  }
	      }

	    /* now fix the ordering in active_segs */
	    asi = active_segs[i];
	    for (j = i; j > target; j--)
	      active_segs[j] = active_segs[j - 1];
	    active_segs[j] = asi;
	  }
      }
    }
  if (swap && start > 0)
    {
      int as_start;

      as_start = active_segs[start];
      if (cursor[as_start] < vp->segs[as_start].n_points)
	{
#ifdef VERBOSE
	  printf ("checking intersection of %d, %d\n", start - 1, start);
#endif
	  intersect_neighbors (start, active_segs,
			       n_ips, n_ips_max, ips,
			       cursor, vp);
	}
    }

  if (swap && end < n_active_segs)
    {
      int as_end;

      as_end = active_segs[end - 1];
      if (cursor[as_end] < vp->segs[as_end].n_points)
	{
#ifdef VERBOSE
	  printf ("checking intersection of %d, %d\n", end - 1, end);
#endif
	  intersect_neighbors (end, active_segs,
			       n_ips, n_ips_max, ips,
			       cursor, vp);
	}
    }
  if (swap)
    {
#ifdef VERBOSE
      printf ("fix_crossing return: [%d..%d)", start, end);
      for (k = 0; k < n_active_segs; k++)
	printf (" %d", active_segs[k]);
      printf ("\n");
#endif
    }
}

/* Return a new sorted vector that covers the same area as the
   argument, but which satisfies the nocross invariant.

   Basically, this routine works by finding the intersection points,
   and cutting the segments at those points.

   Status of this routine:

   Basic correctness: Seems ok.

   Numerical stability: known problems in the case of points falling
   on lines, and colinear lines. For actual use, randomly perturbing
   the vertices is currently recommended.

   Speed: pretty good, although a more efficient priority queue, as
   well as bbox culling of potential intersections, are two
   optimizations that could help.

   Precision: pretty good, although the numerical stability problems
   make this routine unsuitable for precise calculations of
   differences.

*/

/* Here is a more detailed description of the algorithm. It follows
   roughly the structure of traverse (above), but is obviously quite
   a bit more complex.

   Here are a few important data structures:

   A new sorted vector path (new_svp).

   For each (active) segment in the original, a list of intersection
   points.

   Of course, the original being traversed.

   The following invariants hold (in addition to the invariants
   of the traverse procedure).

   The new sorted vector path lies entirely above the y scan line.

   The new sorted vector path keeps the nocross invariant.

   For each active segment, the y scan line crosses the line from the
   first to the second of the intersection points (where the second
   point is cursor + 1 if there is only one intersection point).

   The list of intersection points + the (cursor + 1) point is kept
   in nondecreasing y order.

   Of the active segments, none of the lines from first to second
   intersection point cross the 1st ip..2nd ip line of the left or
   right neighbor. (However, such a line may cross further
   intersection points of the neighbors, or segments past the
   immediate neighbors).

   Of the active segments, all lines from 1st ip..2nd ip are in
   strictly increasing x_order (this is very similar to the invariant
   of the traverse procedure, but is explicitly stated here in terms
   of ips). (this basically says that nocross holds on the active
   segments)

   The combination of the new sorted vector path, the path through all
   the intersection points to cursor + 1, and [cursor + 1, n_points)
   covers the same area as the argument.

   Another important data structure is mapping from original segment
   number to new segment number.

   The algorithm is perhaps best understood as advancing the cursors
   while maintaining these invariants. Here's roughly how it's done.

   When deleting segments from the active list, those segments are added
   to the new sorted vector path. In addition, the neighbors may intersect
   each other, so they are intersection tested (see below).

   When inserting new segments, they are intersection tested against
   their neighbors. The top point of the segment becomes the first
   intersection point.

   Advancing the cursor is just a bit different from the traverse
   routine, as the cursor may advance through the intersection points
   as well. Only when there is a single intersection point in the list
   does the cursor advance in the original segment. In either case,
   the new vector is intersection tested against both neighbors. It
   also causes the vector over which the cursor is advancing to be
   added to the new svp.

   Two steps need further clarification:

   Intersection testing: the 1st ip..2nd ip lines of the neighbors
   are tested to see if they cross (using intersect_lines). If so,
   then the intersection point is added to the ip list of both
   segments, maintaining the invariant that the list of intersection
   points is nondecreasing in y).

   Adding vector to new svp: if the new vector shares a top x
   coordinate with another vector, then it is checked to see whether
   it is in order. If not, then both segments are "broken," and then
   restarted. Note: in the case when both segments are in the same
   order, they may simply be swapped without breaking.

   For the time being, I'm going to put some of these operations into
   subroutines. If it turns out to be a performance problem, I could
   try to reorganize the traverse procedure so that each is only
   called once, and inline them. But if it's not a performance
   problem, I'll just keep it this way, because it will probably help
   to make the code clearer, and I believe this code could use all the
   clarity it can get. */
/**
 * art_svp_uncross: Resolve self-intersections of an svp.
 * @vp: The original svp.
 *
 * Finds all the intersections within @vp, and constructs a new svp
 * with new points added at these intersections.
 *
 * This routine needs to be redone from scratch with numerical robustness
 * in mind. I'm working on it.
 *
 * Return value: The new svp.
 **/
ArtSVP *
art_svp_uncross (ArtSVP *vp)
{
  int *active_segs;
  int n_active_segs;
  int *cursor;
  int seg_idx;
  double y;
  int tmp1, tmp2;
  int asi;
  int i, j;
  /* new data structures */
  /* intersection points; invariant: *ips[i] is only allocated if
     i is active */
  int *n_ips, *n_ips_max;
  ArtPoint **ips;
  /* new sorted vector path */
  int n_segs_max, seg_num;
  ArtSVP *new_vp;
  int *n_points_max;
  /* mapping from argument to new segment numbers - again, only valid
   if active */
  int *seg_map;
  double y_curs;
  ArtPoint p_curs;
  int first_share;
  double share_x;
  ArtPoint *pts;

  n_segs_max = 16;
  new_vp = (ArtSVP *)art_alloc (sizeof(ArtSVP) +
				(n_segs_max - 1) * sizeof(ArtSVPSeg));
  new_vp->n_segs = 0;

  if (vp->n_segs == 0)
    return new_vp;

  active_segs = art_new (int, vp->n_segs);
  cursor = art_new (int, vp->n_segs);

  seg_map = art_new (int, vp->n_segs);
  n_ips = art_new (int, vp->n_segs);
  n_ips_max = art_new (int, vp->n_segs);
  ips = art_new (ArtPoint *, vp->n_segs);

  n_points_max = art_new (int, n_segs_max);

  n_active_segs = 0;
  seg_idx = 0;
  y = vp->segs[0].points[0].y;
  while (seg_idx < vp->n_segs || n_active_segs > 0)
    {
#ifdef VERBOSE
      printf ("y = %g\n", y);
#endif

      /* maybe move deletions to end of loop (to avoid so much special
	 casing on the end of a segment)? */

      /* delete segments ending at y from active list */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  if (vp->segs[asi].n_points - 1 == cursor[asi] &&
	      vp->segs[asi].points[cursor[asi]].y == y)
	    {
	      do
		{
#ifdef VERBOSE
		  printf ("deleting %d\n", asi);
#endif
		  art_free (ips[asi]);
		  n_active_segs--;
		  for (j = i; j < n_active_segs; j++)
		    active_segs[j] = active_segs[j + 1];
		  if (i < n_active_segs)
		    asi = active_segs[i];
		  else
		    break;
		}
	      while (vp->segs[asi].n_points - 1 == cursor[asi] &&
		     vp->segs[asi].points[cursor[asi]].y == y);

	      /* test intersection of neighbors */
	      if (i > 0 && i < n_active_segs)
		intersect_neighbors (i, active_segs,
				     n_ips, n_ips_max, ips,
				     cursor, vp);

	      i--;
	    }	      
	}

      /* insert new segments into the active list */
      while (seg_idx < vp->n_segs && y == vp->segs[seg_idx].points[0].y)
	{
#ifdef VERBOSE
	  printf ("inserting %d\n", seg_idx);
#endif
	  cursor[seg_idx] = 0;
	  for (i = 0; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (x_order_2 (vp->segs[seg_idx].points[0],
			     vp->segs[seg_idx].points[1],
			     vp->segs[asi].points[cursor[asi]],
			     vp->segs[asi].points[cursor[asi] + 1]) == -1)
		break;
	    }

	  /* Create and initialize the intersection points data structure */
	  n_ips[seg_idx] = 1;
	  n_ips_max[seg_idx] = 2;
	  ips[seg_idx] = art_new (ArtPoint, n_ips_max[seg_idx]);
	  ips[seg_idx][0] = vp->segs[seg_idx].points[0];

	  /* Start a new segment in the new vector path */
	  pts = art_new (ArtPoint, 16);
	  pts[0] = vp->segs[seg_idx].points[0];
	  seg_num = art_svp_add_segment (&new_vp, &n_segs_max,
					 &n_points_max,
					 1, vp->segs[seg_idx].dir,
					 pts,
					 NULL);
	  n_points_max[seg_num] = 16;
	  seg_map[seg_idx] = seg_num;

	  tmp1 = seg_idx;
	  for (j = i; j < n_active_segs; j++)
	    {
	      tmp2 = active_segs[j];
	      active_segs[j] = tmp1;
	      tmp1 = tmp2;
	    }
	  active_segs[n_active_segs] = tmp1;
	  n_active_segs++;

	  if (i > 0)
	    intersect_neighbors (i, active_segs,
				 n_ips, n_ips_max, ips,
				 cursor, vp);

	  if (i + 1 < n_active_segs)
	    intersect_neighbors (i + 1, active_segs,
				 n_ips, n_ips_max, ips,
				 cursor, vp);

	  seg_idx++;
	}

      /* all active segs cross the y scanline (considering segs to be
       closed on top and open on bottom) */
#ifdef VERBOSE
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  printf ("%d ", asi);
	  for (j = 0; j < n_ips[asi]; j++)
	    printf ("(%g, %g) - ",
		    ips[asi][j].x,
		    ips[asi][j].y);
	  printf ("(%g, %g) %s\n",
		  vp->segs[asi].points[cursor[asi] + 1].x,
		  vp->segs[asi].points[cursor[asi] + 1].y,
		  vp->segs[asi].dir ? "v" : "^");
	}
#endif

      /* advance y to the next event 
       Note: this is quadratic. We'd probably get decent constant
       factor speed improvement by caching the y_curs values. */
      if (n_active_segs == 0)
	{
	  if (seg_idx < vp->n_segs)
	    y = vp->segs[seg_idx].points[0].y;
	  /* else we're done */
	}
      else
	{
	  asi = active_segs[0];
	  if (n_ips[asi] == 1)
	    y = vp->segs[asi].points[cursor[asi] + 1].y;
	  else
	    y = ips[asi][1].y;
	  for (i = 1; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (n_ips[asi] == 1)
		y_curs = vp->segs[asi].points[cursor[asi] + 1].y;
	      else
		y_curs = ips[asi][1].y;
	      if (y > y_curs)
		y = y_curs;
	    }
	  if (seg_idx < vp->n_segs && y > vp->segs[seg_idx].points[0].y)
	    y = vp->segs[seg_idx].points[0].y;
	}

      first_share = -1;
      share_x = 0; /* to avoid gcc warning, although share_x is never
		      used when first_share is -1 */
      /* advance cursors to reach new y */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  if (n_ips[asi] == 1)
	    p_curs = vp->segs[asi].points[cursor[asi] + 1];
	  else
	    p_curs = ips[asi][1];
	  if (p_curs.y == y)
	    {
	      svp_add_point (new_vp, n_points_max,
			     p_curs, seg_map, active_segs, n_active_segs, i);

	      n_ips[asi]--;
	      for (j = 0; j < n_ips[asi]; j++)
		ips[asi][j] = ips[asi][j + 1];

	      if (n_ips[asi] == 0)
		{
		  ips[asi][0] = p_curs;
		  n_ips[asi] = 1;
		  cursor[asi]++;
		}

	      if (first_share < 0 || p_curs.x != share_x)
		{
		  /* this is where crossings are detected, and if
		     found, the active segments switched around. */
		      
		  fix_crossing (first_share, i,
				active_segs, n_active_segs,
				cursor, ips, n_ips, n_ips_max, vp, seg_map,
				&new_vp,
				&n_segs_max, &n_points_max);

		  first_share = i;
		  share_x = p_curs.x;
		}

	      if (cursor[asi] < vp->segs[asi].n_points - 1)
		{

		  if (i > 0)
		    intersect_neighbors (i, active_segs,
					 n_ips, n_ips_max, ips,
					 cursor, vp);
		  
		  if (i + 1 < n_active_segs)
		    intersect_neighbors (i + 1, active_segs,
					 n_ips, n_ips_max, ips,
					 cursor, vp);
		}
	    }
	  else
	    {
	      /* not on a cursor point */
	      fix_crossing (first_share, i,
			    active_segs, n_active_segs,
			    cursor, ips, n_ips, n_ips_max, vp, seg_map,
			    &new_vp,
			    &n_segs_max, &n_points_max);
	      first_share = -1;
	    }
	}

      /* fix crossing on last shared group */
      fix_crossing (first_share, i,
		    active_segs, n_active_segs,
		    cursor, ips, n_ips, n_ips_max, vp, seg_map,
		    &new_vp,
		    &n_segs_max, &n_points_max);

#ifdef VERBOSE
      printf ("\n");
#endif
    }

  /* not necessary to sort, new segments only get added at y, which
     increases monotonically */
#if 0
  qsort (&new_vp->segs, new_vp->n_segs, sizeof (svp_seg), svp_seg_compare);
  {
    int k;
    for (k = 0; k < new_vp->n_segs - 1; k++)
      {
	printf ("(%g, %g) - (%g, %g) %s (%g, %g) - (%g, %g)\n",
		new_vp->segs[k].points[0].x,
		new_vp->segs[k].points[0].y,
		new_vp->segs[k].points[1].x,
		new_vp->segs[k].points[1].y,
		svp_seg_compare (&new_vp->segs[k], &new_vp->segs[k + 1]) > 1 ? ">": "<",
		new_vp->segs[k + 1].points[0].x,
		new_vp->segs[k + 1].points[0].y,
		new_vp->segs[k + 1].points[1].x,
		new_vp->segs[k + 1].points[1].y);
      }
  }
#endif

  art_free (n_points_max);
  art_free (seg_map);
  art_free (n_ips_max);
  art_free (n_ips);
  art_free (ips);
  art_free (cursor);
  art_free (active_segs);

  return new_vp;
}

#define noVERBOSE

/* Rewind a svp satisfying the nocross invariant.

   The winding number of a segment is defined as the winding number of
   the points to the left while travelling in the direction of the
   segment. Therefore it preincrements and postdecrements as a scan
   line is traversed from left to right.

   Status of this routine:

   Basic correctness: Was ok in gfonted. However, this code does not
   yet compute bboxes for the resulting svp segs.

   Numerical stability: known problems in the case of horizontal
   segments in polygons with any complexity. For actual use, randomly
   perturbing the vertices is recommended.

   Speed: good.

   Precision: good, except that no attempt is made to remove "hair".
   Doing random perturbation just makes matters worse.

*/
/**
 * art_svp_rewind_uncrossed: Rewind an svp satisfying the nocross invariant.
 * @vp: The original svp.
 * @rule: The winding rule.
 *
 * Creates a new svp with winding number of 0 or 1 everywhere. The @rule
 * argument specifies a rule for how winding numbers in the original
 * @vp map to the winding numbers in the result.
 *
 * With @rule == ART_WIND_RULE_NONZERO, the resulting svp has a
 * winding number of 1 where @vp has a nonzero winding number.
 *
 * With @rule == ART_WIND_RULE_INTERSECT, the resulting svp has a
 * winding number of 1 where @vp has a winding number greater than
 * 1. It is useful for computing intersections.
 *
 * With @rule == ART_WIND_RULE_ODDEVEN, the resulting svp has a
 * winding number of 1 where @vp has an odd winding number. It is
 * useful for implementing the even-odd winding rule of the
 * PostScript imaging model.
 *
 * With @rule == ART_WIND_RULE_POSITIVE, the resulting svp has a
 * winding number of 1 where @vp has a positive winding number. It is
 * usefull for implementing asymmetric difference.
 *
 * This routine needs to be redone from scratch with numerical robustness
 * in mind. I'm working on it.
 *
 * Return value: The new svp.
 **/
ArtSVP *
art_svp_rewind_uncrossed (ArtSVP *vp, ArtWindRule rule)
{
  int *active_segs;
  int n_active_segs;
  int *cursor;
  int seg_idx;
  double y;
  int tmp1, tmp2;
  int asi;
  int i, j;

  ArtSVP *new_vp;
  int n_segs_max;
  int *winding;
  int left_wind;
  int wind;
  int keep, invert;

#ifdef VERBOSE
  print_svp (vp);
#endif
  n_segs_max = 16;
  new_vp = (ArtSVP *)art_alloc (sizeof(ArtSVP) +
				(n_segs_max - 1) * sizeof(ArtSVPSeg));
  new_vp->n_segs = 0;

  if (vp->n_segs == 0)
    return new_vp;

  winding = art_new (int, vp->n_segs);

  active_segs = art_new (int, vp->n_segs);
  cursor = art_new (int, vp->n_segs);

  n_active_segs = 0;
  seg_idx = 0;
  y = vp->segs[0].points[0].y;
  while (seg_idx < vp->n_segs || n_active_segs > 0)
    {
#ifdef VERBOSE
      printf ("y = %g\n", y);
#endif
      /* delete segments ending at y from active list */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  if (vp->segs[asi].n_points - 1 == cursor[asi] &&
	      vp->segs[asi].points[cursor[asi]].y == y)
	    {
#ifdef VERBOSE
	      printf ("deleting %d\n", asi);
#endif
	      n_active_segs--;
	      for (j = i; j < n_active_segs; j++)
		active_segs[j] = active_segs[j + 1];
	      i--;
	    }
	}

      /* insert new segments into the active list */
      while (seg_idx < vp->n_segs && y == vp->segs[seg_idx].points[0].y)
	{
#ifdef VERBOSE
	  printf ("inserting %d\n", seg_idx);
#endif
	  cursor[seg_idx] = 0;
	  for (i = 0; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (x_order_2 (vp->segs[seg_idx].points[0],
			     vp->segs[seg_idx].points[1],
			     vp->segs[asi].points[cursor[asi]],
			     vp->segs[asi].points[cursor[asi] + 1]) == -1)
		break;
	    }

	  /* Determine winding number for this segment */
	  if (i == 0)
	    left_wind = 0;
	  else if (vp->segs[active_segs[i - 1]].dir)
	    left_wind = winding[active_segs[i - 1]];
	  else
	    left_wind = winding[active_segs[i - 1]] - 1;

	  if (vp->segs[seg_idx].dir)
	    wind = left_wind + 1;
	  else
	    wind = left_wind;

	  winding[seg_idx] = wind;

	  switch (rule)
	    {
	    case ART_WIND_RULE_NONZERO:
	      keep = (wind == 1 || wind == 0);
	      invert = (wind == 0);
	      break;
	    case ART_WIND_RULE_INTERSECT:
	      keep = (wind == 2);
	      invert = 0;
	      break;
	    case ART_WIND_RULE_ODDEVEN:
	      keep = 1;
	      invert = !(wind & 1);
	      break;
	    case ART_WIND_RULE_POSITIVE:
	      keep = (wind == 1);
	      invert = 0;
	      break;
	    default:
	      keep = 0;
	      invert = 0;
	      break;
	    }

	  if (keep)
	    {
	      ArtPoint *points, *new_points;
	      int n_points;
	      int new_dir;

#ifdef VERBOSE
	      printf ("keeping segment %d\n", seg_idx);
#endif
	      n_points = vp->segs[seg_idx].n_points;
	      points = vp->segs[seg_idx].points;
	      new_points = art_new (ArtPoint, n_points);
	      memcpy (new_points, points, n_points * sizeof (ArtPoint));
	      new_dir = vp->segs[seg_idx].dir ^ invert;
	      art_svp_add_segment (&new_vp, &n_segs_max,
				   NULL,
				   n_points, new_dir, new_points,
				   &vp->segs[seg_idx].bbox);
	    }

	  tmp1 = seg_idx;
	  for (j = i; j < n_active_segs; j++)
	    {
	      tmp2 = active_segs[j];
	      active_segs[j] = tmp1;
	      tmp1 = tmp2;
	    }
	  active_segs[n_active_segs] = tmp1;
	  n_active_segs++;
	  seg_idx++;
	}

#ifdef VERBOSE
      /* all active segs cross the y scanline (considering segs to be
       closed on top and open on bottom) */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  printf ("%d:%d (%g, %g) - (%g, %g) %s %d\n", asi,
		  cursor[asi],
		  vp->segs[asi].points[cursor[asi]].x,
		  vp->segs[asi].points[cursor[asi]].y,
		  vp->segs[asi].points[cursor[asi] + 1].x,
		  vp->segs[asi].points[cursor[asi] + 1].y,
		  vp->segs[asi].dir ? "v" : "^",
		  winding[asi]);
	}
#endif

      /* advance y to the next event */
      if (n_active_segs == 0)
	{
	  if (seg_idx < vp->n_segs)
	    y = vp->segs[seg_idx].points[0].y;
	  /* else we're done */
	}
      else
	{
	  asi = active_segs[0];
	  y = vp->segs[asi].points[cursor[asi] + 1].y;
	  for (i = 1; i < n_active_segs; i++)
	    {
	      asi = active_segs[i];
	      if (y > vp->segs[asi].points[cursor[asi] + 1].y)
		y = vp->segs[asi].points[cursor[asi] + 1].y;
	    }
	  if (seg_idx < vp->n_segs && y > vp->segs[seg_idx].points[0].y)
	    y = vp->segs[seg_idx].points[0].y;
	}

      /* advance cursors to reach new y */
      for (i = 0; i < n_active_segs; i++)
	{
	  asi = active_segs[i];
	  while (cursor[asi] < vp->segs[asi].n_points - 1 &&
		 y >= vp->segs[asi].points[cursor[asi] + 1].y)
	    cursor[asi]++;
	}
#ifdef VERBOSE
      printf ("\n");
#endif
    }
  art_free (cursor);
  art_free (active_segs);
  art_free (winding);

  return new_vp;
}
