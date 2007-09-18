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


#include <stdlib.h>
#include <math.h>

#include "art_misc.h"

#include "art_vpath.h"
#include "art_svp.h"
#include "art_svp_wind.h"
#include "art_svp_vpath.h"
#include "art_svp_vpath_stroke.h"

#define EPSILON 1e-6
#define EPSILON_2 1e-12

#define yes_OPTIMIZE_INNER

/* Render an arc segment starting at (xc + x0, yc + y0) to (xc + x1,
   yc + y1), centered at (xc, yc), and with given radius. Both x0^2 +
   y0^2 and x1^2 + y1^2 should be equal to radius^2.

   A positive value of radius means curve to the left, negative means
   curve to the right.
*/
static void
art_svp_vpath_stroke_arc (ArtVpath **p_vpath, int *pn, int *pn_max,
			  double xc, double yc,
			  double x0, double y0,
			  double x1, double y1,
			  double radius,
			  double flatness)
{
  double theta;
  double th_0, th_1;
  int n_pts;
  int i;
  double aradius;

  aradius = fabs (radius);
  theta = 2 * M_SQRT2 * sqrt (flatness / aradius);
  th_0 = atan2 (y0, x0);
  th_1 = atan2 (y1, x1);
  if (radius > 0)
    {
      /* curve to the left */
      if (th_0 < th_1) th_0 += M_PI * 2;
      n_pts = ceil ((th_0 - th_1) / theta);
    }
  else
    {
      /* curve to the right */
      if (th_1 < th_0) th_1 += M_PI * 2;
      n_pts = ceil ((th_1 - th_0) / theta);
    }
#ifdef VERBOSE
  printf ("start %f %f; th_0 = %f, th_1 = %f, r = %f, theta = %f\n", x0, y0, th_0, th_1, radius, theta);
#endif
  art_vpath_add_point (p_vpath, pn, pn_max,
		       ART_LINETO, xc + x0, yc + y0);
  for (i = 1; i < n_pts; i++)
    {
      theta = th_0 + (th_1 - th_0) * i / n_pts;
      art_vpath_add_point (p_vpath, pn, pn_max,
			   ART_LINETO, xc + cos (theta) * aradius,
			   yc + sin (theta) * aradius);
#ifdef VERBOSE
      printf ("mid %f %f\n", cos (theta) * radius, sin (theta) * radius);
#endif
    }
  art_vpath_add_point (p_vpath, pn, pn_max,
		       ART_LINETO, xc + x1, yc + y1);
#ifdef VERBOSE
  printf ("end %f %f\n", x1, y1);
#endif
}

/* Assume that forw and rev are at point i0. Bring them to i1,
   joining with the vector i1 - i2.

   This used to be true, but isn't now that the stroke_raw code is
   filtering out (near)zero length vectors: {It so happens that all
   invocations of this function maintain the precondition i1 = i0 + 1,
   so we could decrease the number of arguments by one. We haven't
   done that here, though.}

   forw is to the line's right and rev is to its left.

   Precondition: no zero-length vectors, otherwise a divide by
   zero will happen.  */
static void
render_seg (ArtVpath **p_forw, int *pn_forw, int *pn_forw_max,
	    ArtVpath **p_rev, int *pn_rev, int *pn_rev_max,
	    ArtVpath *vpath, int i0, int i1, int i2,
	    ArtPathStrokeJoinType join,
	    double line_width, double miter_limit, double flatness)
{
  double dx0, dy0;
  double dx1, dy1;
  double dlx0, dly0;
  double dlx1, dly1;
  double dmx, dmy;
  double dmr2;
  double scale;
  double cross;

#ifdef VERBOSE
  printf ("join style = %d\n", join);
#endif

  /* The vectors of the lines from i0 to i1 and i1 to i2. */
  dx0 = vpath[i1].x - vpath[i0].x;
  dy0 = vpath[i1].y - vpath[i0].y;

  dx1 = vpath[i2].x - vpath[i1].x;
  dy1 = vpath[i2].y - vpath[i1].y;

  /* Set dl[xy]0 to the vector from i0 to i1, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx0 * dx0 + dy0 * dy0);
  dlx0 = dy0 * scale;
  dly0 = -dx0 * scale;

  /* Set dl[xy]1 to the vector from i1 to i2, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx1 * dx1 + dy1 * dy1);
  dlx1 = dy1 * scale;
  dly1 = -dx1 * scale;

#ifdef VERBOSE
  printf ("%% render_seg: (%g, %g) - (%g, %g) - (%g, %g)\n",
	  vpath[i0].x, vpath[i0].y,
	  vpath[i1].x, vpath[i1].y,
	  vpath[i2].x, vpath[i2].y);

  printf ("%% render_seg: d[xy]0 = (%g, %g), dl[xy]0 = (%g, %g)\n",
	  dx0, dy0, dlx0, dly0);

  printf ("%% render_seg: d[xy]1 = (%g, %g), dl[xy]1 = (%g, %g)\n",
	  dx1, dy1, dlx1, dly1);
#endif

  /* now, forw's last point is expected to be colinear along d[xy]0
     to point i0 - dl[xy]0, and rev with i0 + dl[xy]0. */

  /* positive for positive area (i.e. left turn) */
  cross = dx1 * dy0 - dx0 * dy1;

  dmx = (dlx0 + dlx1) * 0.5;
  dmy = (dly0 + dly1) * 0.5;
  dmr2 = dmx * dmx + dmy * dmy;

  if (join == ART_PATH_STROKE_JOIN_MITER &&
      dmr2 * miter_limit * miter_limit < line_width * line_width)
    join = ART_PATH_STROKE_JOIN_BEVEL;

  /* the case when dmr2 is zero or very small bothers me
     (i.e. near a 180 degree angle) */
  scale = line_width * line_width / dmr2;
  dmx *= scale;
  dmy *= scale;

  if (cross * cross < EPSILON_2 && dx0 * dx1 + dy0 * dy1 >= 0)
    {
      /* going straight */
#ifdef VERBOSE
      printf ("%% render_seg: straight\n");
#endif
      art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
		       ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
      art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
		       ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
    }
  else if (cross > 0)
    {
      /* left turn, forw is outside and rev is inside */

#ifdef VERBOSE
      printf ("%% render_seg: left\n");
#endif
      if (
#ifdef NO_OPTIMIZE_INNER
	  0 &&
#endif
	  /* check that i1 + dm[xy] is inside i0-i1 rectangle */
	  (dx0 + dmx) * dx0 + (dy0 + dmy) * dy0 > 0 &&
	  /* and that i1 + dm[xy] is inside i1-i2 rectangle */
	  ((dx1 - dmx) * dx1 + (dy1 - dmy) * dy1 > 0)
#ifdef PEDANTIC_INNER
	  &&
	  /* check that i1 + dl[xy]1 is inside i0-i1 rectangle */
	  (dx0 + dlx1) * dx0 + (dy0 + dly1) * dy0 > 0 &&
	  /* and that i1 + dl[xy]0 is inside i1-i2 rectangle */
	  ((dx1 - dlx0) * dx1 + (dy1 - dly0) * dy1 > 0)
#endif
	  )
	{
	  /* can safely add single intersection point */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dmx, vpath[i1].y + dmy);
	}
      else
	{
	  /* need to loop-de-loop the inside */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x, vpath[i1].y);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx1, vpath[i1].y + dly1);
	}

      if (join == ART_PATH_STROKE_JOIN_BEVEL)
	{
	  /* bevel */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx1, vpath[i1].y - dly1);
	}
      else if (join == ART_PATH_STROKE_JOIN_MITER)
	{
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dmx, vpath[i1].y - dmy);
	}
      else if (join == ART_PATH_STROKE_JOIN_ROUND)
	art_svp_vpath_stroke_arc (p_forw, pn_forw, pn_forw_max,
				  vpath[i1].x, vpath[i1].y,
				  -dlx0, -dly0,
				  -dlx1, -dly1,
				  line_width,
				  flatness);
    }
  else
    {
      /* right turn, rev is outside and forw is inside */
#ifdef VERBOSE
      printf ("%% render_seg: right\n");
#endif

      if (
#ifdef NO_OPTIMIZE_INNER
	  0 &&
#endif
	  /* check that i1 - dm[xy] is inside i0-i1 rectangle */
	  (dx0 - dmx) * dx0 + (dy0 - dmy) * dy0 > 0 &&
	  /* and that i1 - dm[xy] is inside i1-i2 rectangle */
	  ((dx1 + dmx) * dx1 + (dy1 + dmy) * dy1 > 0)
#ifdef PEDANTIC_INNER
	  &&
	  /* check that i1 - dl[xy]1 is inside i0-i1 rectangle */
	  (dx0 - dlx1) * dx0 + (dy0 - dly1) * dy0 > 0 &&
	  /* and that i1 - dl[xy]0 is inside i1-i2 rectangle */
	  ((dx1 + dlx0) * dx1 + (dy1 + dly0) * dy1 > 0)
#endif
	  )
	{
	  /* can safely add single intersection point */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dmx, vpath[i1].y - dmy);
	}
      else
	{
	  /* need to loop-de-loop the inside */
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x, vpath[i1].y);
	  art_vpath_add_point (p_forw, pn_forw, pn_forw_max,
			   ART_LINETO, vpath[i1].x - dlx1, vpath[i1].y - dly1);
	}

      if (join == ART_PATH_STROKE_JOIN_BEVEL)
	{
	  /* bevel */
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dlx1, vpath[i1].y + dly1);
	}
      else if (join == ART_PATH_STROKE_JOIN_MITER)
	{
	  art_vpath_add_point (p_rev, pn_rev, pn_rev_max,
			   ART_LINETO, vpath[i1].x + dmx, vpath[i1].y + dmy);
	}
      else if (join == ART_PATH_STROKE_JOIN_ROUND)
	art_svp_vpath_stroke_arc (p_rev, pn_rev, pn_rev_max,
				  vpath[i1].x, vpath[i1].y,
				  dlx0, dly0,
				  dlx1, dly1,
				  -line_width,
				  flatness);

    }
}

/* caps i1, under the assumption of a vector from i0 */
static void
render_cap (ArtVpath **p_result, int *pn_result, int *pn_result_max,
	    ArtVpath *vpath, int i0, int i1,
	    ArtPathStrokeCapType cap, double line_width, double flatness)
{
  double dx0, dy0;
  double dlx0, dly0;
  double scale;
  int n_pts;
  int i;

  dx0 = vpath[i1].x - vpath[i0].x;
  dy0 = vpath[i1].y - vpath[i0].y;

  /* Set dl[xy]0 to the vector from i0 to i1, rotated counterclockwise
     90 degrees, and scaled to the length of line_width. */
  scale = line_width / sqrt (dx0 * dx0 + dy0 * dy0);
  dlx0 = dy0 * scale;
  dly0 = -dx0 * scale;

#ifdef VERBOSE
  printf ("cap style = %d\n", cap);
#endif

  switch (cap)
    {
    case ART_PATH_STROKE_CAP_BUTT:
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
      break;
    case ART_PATH_STROKE_CAP_ROUND:
      n_pts = ceil (M_PI / (2.0 * M_SQRT2 * sqrt (flatness / line_width)));
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO, vpath[i1].x - dlx0, vpath[i1].y - dly0);
      for (i = 1; i < n_pts; i++)
	{
	  double theta, c_th, s_th;

	  theta = M_PI * i / n_pts;
	  c_th = cos (theta);
	  s_th = sin (theta);
	  art_vpath_add_point (p_result, pn_result, pn_result_max,
			       ART_LINETO,
			       vpath[i1].x - dlx0 * c_th - dly0 * s_th,
			       vpath[i1].y - dly0 * c_th + dlx0 * s_th);
	}
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO, vpath[i1].x + dlx0, vpath[i1].y + dly0);
      break;
    case ART_PATH_STROKE_CAP_SQUARE:
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO,
			   vpath[i1].x - dlx0 - dly0,
			   vpath[i1].y - dly0 + dlx0);
      art_vpath_add_point (p_result, pn_result, pn_result_max,
			   ART_LINETO,
			   vpath[i1].x + dlx0 - dly0,
			   vpath[i1].y + dly0 + dlx0);
      break;
    }
}

/**
 * art_svp_from_vpath_raw: Stroke a vector path, raw version
 * @vpath: #ArtVPath to stroke.
 * @join: Join style.
 * @cap: Cap style.
 * @line_width: Width of stroke.
 * @miter_limit: Miter limit.
 * @flatness: Flatness.
 *
 * Exactly the same as art_svp_vpath_stroke(), except that the resulting
 * stroke outline may self-intersect and have regions of winding number
 * greater than 1.
 *
 * Return value: Resulting raw stroked outline in svp format.
 **/
ArtVpath *
art_svp_vpath_stroke_raw (ArtVpath *vpath,
			  ArtPathStrokeJoinType join,
			  ArtPathStrokeCapType cap,
			  double line_width,
			  double miter_limit,
			  double flatness)
{
  int begin_idx, end_idx;
  int i;
  ArtVpath *forw, *rev;
  int n_forw, n_rev;
  int n_forw_max, n_rev_max;
  ArtVpath *result;
  int n_result, n_result_max;
  double half_lw = 0.5 * line_width;
  int closed;
  int last, this, next, second;
  double dx, dy;

  n_forw_max = 16;
  forw = art_new (ArtVpath, n_forw_max);

  n_rev_max = 16;
  rev = art_new (ArtVpath, n_rev_max);

  n_result = 0;
  n_result_max = 16;
  result = art_new (ArtVpath, n_result_max);

  for (begin_idx = 0; vpath[begin_idx].code != ART_END; begin_idx = end_idx)
    {
      n_forw = 0;
      n_rev = 0;

      closed = (vpath[begin_idx].code == ART_MOVETO);

      /* we don't know what the first point joins with until we get to the
	 last point and see if it's closed. So we start with the second
	 line in the path.

	 Note: this is not strictly true (we now know it's closed from
	 the opening pathcode), but why fix code that isn't broken?
      */

      this = begin_idx;
      /* skip over identical points at the beginning of the subpath */
      for (i = this + 1; vpath[i].code == ART_LINETO; i++)
	{
	  dx = vpath[i].x - vpath[this].x;
	  dy = vpath[i].y - vpath[this].y;
	  if (dx * dx + dy * dy > EPSILON_2)
	    break;
	}
      next = i;
      second = next;

      /* invariant: this doesn't coincide with next */
      while (vpath[next].code == ART_LINETO)
	{
	  last = this;
	  this = next;
	  /* skip over identical points after the beginning of the subpath */
	  for (i = this + 1; vpath[i].code == ART_LINETO; i++)
	    {
	      dx = vpath[i].x - vpath[this].x;
	      dy = vpath[i].y - vpath[this].y;
	      if (dx * dx + dy * dy > EPSILON_2)
		break;
	    }
	  next = i;
	  if (vpath[next].code != ART_LINETO)
	    {
	      /* reached end of path */
	      /* make "closed" detection conform to PostScript
		 semantics (i.e. explicit closepath code rather than
		 just the fact that end of the path is the beginning) */
	      if (closed &&
		  vpath[this].x == vpath[begin_idx].x &&
		  vpath[this].y == vpath[begin_idx].y)
		{
		  int j;

		  /* path is closed, render join to beginning */
		  render_seg (&forw, &n_forw, &n_forw_max,
			      &rev, &n_rev, &n_rev_max,
			      vpath, last, this, second,
			      join, half_lw, miter_limit, flatness);

#ifdef VERBOSE
		  printf ("%% forw %d, rev %d\n", n_forw, n_rev);
#endif
		  /* do forward path */
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, forw[n_forw - 1].x,
				   forw[n_forw - 1].y);
		  for (j = 0; j < n_forw; j++)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, forw[j].x,
				     forw[j].y);

		  /* do reverse path, reversed */
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, rev[0].x,
				   rev[0].y);
		  for (j = n_rev - 1; j >= 0; j--)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, rev[j].x,
				     rev[j].y);
		}
	      else
		{
		  /* path is open */
		  int j;

		  /* add to forw rather than result to ensure that
		     forw has at least one point. */
		  render_cap (&forw, &n_forw, &n_forw_max,
			      vpath, last, this,
			      cap, half_lw, flatness);
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_MOVETO, forw[0].x,
				   forw[0].y);
		  for (j = 1; j < n_forw; j++)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, forw[j].x,
				     forw[j].y);
		  for (j = n_rev - 1; j >= 0; j--)
		    art_vpath_add_point (&result, &n_result, &n_result_max,
				     ART_LINETO, rev[j].x,
				     rev[j].y);
		  render_cap (&result, &n_result, &n_result_max,
			      vpath, second, begin_idx,
			      cap, half_lw, flatness);
		  art_vpath_add_point (&result, &n_result, &n_result_max,
				   ART_LINETO, forw[0].x,
				   forw[0].y);
		}
	    }
	  else
	    render_seg (&forw, &n_forw, &n_forw_max,
			&rev, &n_rev, &n_rev_max,
			vpath, last, this, next,
			join, half_lw, miter_limit, flatness);
	}
      end_idx = next;
    }

  art_free (forw);
  art_free (rev);
#ifdef VERBOSE
  printf ("%% n_result = %d\n", n_result);
#endif
  art_vpath_add_point (&result, &n_result, &n_result_max, ART_END, 0, 0);
  return result;
}

#define noVERBOSE

#ifdef VERBOSE

#define XOFF 50
#define YOFF 700

static void
print_ps_vpath (ArtVpath *vpath)
{
  int i;

  for (i = 0; vpath[i].code != ART_END; i++)
    {
      switch (vpath[i].code)
	{
	case ART_MOVETO:
	  printf ("%g %g moveto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	case ART_LINETO:
	  printf ("%g %g lineto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	default:
	  break;
	}
    }
  printf ("stroke showpage\n");
}

static void
print_ps_svp (ArtSVP *vpath)
{
  int i, j;

  printf ("%% begin\n");
  for (i = 0; i < vpath->n_segs; i++)
    {
      printf ("%g setgray\n", vpath->segs[i].dir ? 0.7 : 0);
      for (j = 0; j < vpath->segs[i].n_points; j++)
	{
	  printf ("%g %g %s\n",
		  XOFF + vpath->segs[i].points[j].x,
		  YOFF - vpath->segs[i].points[j].y,
		  j ? "lineto" : "moveto");
	}
      printf ("stroke\n");
    }

  printf ("showpage\n");
}
#endif

/* Render a vector path into a stroked outline.

   Status of this routine:

   Basic correctness: Only miter and bevel line joins are implemented,
   and only butt line caps. Otherwise, seems to be fine.

   Numerical stability: We cheat (adding random perturbation). Thus,
   it seems very likely that no numerical stability problems will be
   seen in practice.

   Speed: Should be pretty good.

   Precision: The perturbation fuzzes the coordinates slightly,
   but not enough to be visible.  */
/**
 * art_svp_vpath_stroke: Stroke a vector path.
 * @vpath: #ArtVPath to stroke.
 * @join: Join style.
 * @cap: Cap style.
 * @line_width: Width of stroke.
 * @miter_limit: Miter limit.
 * @flatness: Flatness.
 *
 * Computes an svp representing the stroked outline of @vpath. The
 * width of the stroked line is @line_width.
 *
 * Lines are joined according to the @join rule. Possible values are
 * ART_PATH_STROKE_JOIN_MITER (for mitered joins),
 * ART_PATH_STROKE_JOIN_ROUND (for round joins), and
 * ART_PATH_STROKE_JOIN_BEVEL (for bevelled joins). The mitered join
 * is converted to a bevelled join if the miter would extend to a
 * distance of more than @miter_limit * @line_width from the actual
 * join point.
 *
 * If there are open subpaths, the ends of these subpaths are capped
 * according to the @cap rule. Possible values are
 * ART_PATH_STROKE_CAP_BUTT (squared cap, extends exactly to end
 * point), ART_PATH_STROKE_CAP_ROUND (rounded half-circle centered at
 * the end point), and ART_PATH_STROKE_CAP_SQUARE (squared cap,
 * extending half @line_width past the end point).
 *
 * The @flatness parameter controls the accuracy of the rendering. It
 * is most important for determining the number of points to use to
 * approximate circular arcs for round lines and joins. In general, the
 * resulting vector path will be within @flatness pixels of the "ideal"
 * path containing actual circular arcs. I reserve the right to use
 * the @flatness parameter to convert bevelled joins to miters for very
 * small turn angles, as this would reduce the number of points in the
 * resulting outline path.
 *
 * The resulting path is "clean" with respect to self-intersections, i.e.
 * the winding number is 0 or 1 at each point.
 *
 * Return value: Resulting stroked outline in svp format.
 **/
ArtSVP *
art_svp_vpath_stroke (ArtVpath *vpath,
		      ArtPathStrokeJoinType join,
		      ArtPathStrokeCapType cap,
		      double line_width,
		      double miter_limit,
		      double flatness)
{
  ArtVpath *vpath_stroke, *vpath2;
  ArtSVP *svp, *svp2, *svp3;

  vpath_stroke = art_svp_vpath_stroke_raw (vpath, join, cap,
					   line_width, miter_limit, flatness);
#ifdef VERBOSE
  print_ps_vpath (vpath_stroke);
#endif
  vpath2 = art_vpath_perturb (vpath_stroke);
#ifdef VERBOSE
  print_ps_vpath (vpath2);
#endif
  art_free (vpath_stroke);
  svp = art_svp_from_vpath (vpath2);
#ifdef VERBOSE
  print_ps_svp (svp);
#endif
  art_free (vpath2);
  svp2 = art_svp_uncross (svp);
#ifdef VERBOSE
  print_ps_svp (svp2);
#endif
  art_svp_free (svp);
  svp3 = art_svp_rewind_uncrossed (svp2, ART_WIND_RULE_NONZERO);
#ifdef VERBOSE
  print_ps_svp (svp3);
#endif
  art_svp_free (svp2);

  return svp3;
}
