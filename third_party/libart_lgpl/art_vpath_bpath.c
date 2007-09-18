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

/* Basic constructors and operations for bezier paths */

#include <math.h>

#include "art_misc.h"

#include "art_bpath.h"
#include "art_vpath.h"
#include "art_vpath_bpath.h"

/* p must be allocated 2^level points. */

/* level must be >= 1 */
ArtPoint *
art_bezier_to_vec (double x0, double y0,
		   double x1, double y1,
		   double x2, double y2,
		   double x3, double y3,
		   ArtPoint *p,
		   int level)
{
  double x_m, y_m;

#ifdef VERBOSE
  printf ("bezier_to_vec: %g,%g %g,%g %g,%g %g,%g %d\n",
	  x0, y0, x1, y1, x2, y2, x3, y3, level);
#endif
  if (level == 1) {
    x_m = (x0 + 3 * (x1 + x2) + x3) * 0.125;
    y_m = (y0 + 3 * (y1 + y2) + y3) * 0.125;
    p->x = x_m;
    p->y = y_m;
    p++;
    p->x = x3;
    p->y = y3;
    p++;
#ifdef VERBOSE
    printf ("-> (%g, %g) -> (%g, %g)\n", x_m, y_m, x3, y3);
#endif
  } else {
    double xa1, ya1;
    double xa2, ya2;
    double xb1, yb1;
    double xb2, yb2;

    xa1 = (x0 + x1) * 0.5;
    ya1 = (y0 + y1) * 0.5;
    xa2 = (x0 + 2 * x1 + x2) * 0.25;
    ya2 = (y0 + 2 * y1 + y2) * 0.25;
    xb1 = (x1 + 2 * x2 + x3) * 0.25;
    yb1 = (y1 + 2 * y2 + y3) * 0.25;
    xb2 = (x2 + x3) * 0.5;
    yb2 = (y2 + y3) * 0.5;
    x_m = (xa2 + xb1) * 0.5;
    y_m = (ya2 + yb1) * 0.5;
#ifdef VERBOSE
    printf ("%g,%g %g,%g %g,%g %g,%g\n", xa1, ya1, xa2, ya2,
	    xb1, yb1, xb2, yb2);
#endif
    p = art_bezier_to_vec (x0, y0, xa1, ya1, xa2, ya2, x_m, y_m, p, level - 1);
    p = art_bezier_to_vec (x_m, y_m, xb1, yb1, xb2, yb2, x3, y3, p, level - 1);
  }
  return p;
}

#define RENDER_LEVEL 4
#define RENDER_SIZE (1 << (RENDER_LEVEL))

/**
 * art_vpath_render_bez: Render a bezier segment into the vpath. 
 * @p_vpath: Where the pointer to the #ArtVpath structure is stored.
 * @pn_points: Pointer to the number of points in *@p_vpath.
 * @pn_points_max: Pointer to the number of points allocated.
 * @x0: X coordinate of starting bezier point.
 * @y0: Y coordinate of starting bezier point.
 * @x1: X coordinate of first bezier control point.
 * @y1: Y coordinate of first bezier control point.
 * @x2: X coordinate of second bezier control point.
 * @y2: Y coordinate of second bezier control point.
 * @x3: X coordinate of ending bezier point.
 * @y3: Y coordinate of ending bezier point.
 * @flatness: Flatness control.
 *
 * Renders a bezier segment into the vector path, reallocating and
 * updating *@p_vpath and *@pn_vpath_max as necessary. *@pn_vpath is
 * incremented by the number of vector points added.
 *
 * This step includes (@x0, @y0) but not (@x3, @y3).
 *
 * The @flatness argument guides the amount of subdivision. The Adobe
 * PostScript reference manual defines flatness as the maximum
 * deviation between the any point on the vpath approximation and the
 * corresponding point on the "true" curve, and we follow this
 * definition here. A value of 0.25 should ensure high quality for aa
 * rendering.
**/
static void
art_vpath_render_bez (ArtVpath **p_vpath, int *pn, int *pn_max,
		      double x0, double y0,
		      double x1, double y1,
		      double x2, double y2,
		      double x3, double y3,
		      double flatness)
{
  double x3_0, y3_0;
  double z3_0_dot;
  double z1_dot, z2_dot;
  double z1_perp, z2_perp;
  double max_perp_sq;

  double x_m, y_m;
  double xa1, ya1;
  double xa2, ya2;
  double xb1, yb1;
  double xb2, yb2;

  /* It's possible to optimize this routine a fair amount.

     First, once the _dot conditions are met, they will also be met in
     all further subdivisions. So we might recurse to a different
     routine that only checks the _perp conditions.

     Second, the distance _should_ decrease according to fairly
     predictable rules (a factor of 4 with each subdivision). So it might
     be possible to note that the distance is within a factor of 4 of
     acceptable, and subdivide once. But proving this might be hard.

     Third, at the last subdivision, x_m and y_m can be computed more
     expeditiously (as in the routine above).

     Finally, if we were able to subdivide by, say 2 or 3, this would
     allow considerably finer-grain control, i.e. fewer points for the
     same flatness tolerance. This would speed things up downstream.

     In any case, this routine is unlikely to be the bottleneck. It's
     just that I have this undying quest for more speed...

  */

  x3_0 = x3 - x0;
  y3_0 = y3 - y0;

  /* z3_0_dot is dist z0-z3 squared */
  z3_0_dot = x3_0 * x3_0 + y3_0 * y3_0;

  /* todo: this test is far from satisfactory. */
  if (z3_0_dot < 0.001)
    goto nosubdivide;

  /* we can avoid subdivision if:

     z1 has distance no more than flatness from the z0-z3 line

     z1 is no more z0'ward than flatness past z0-z3

     z1 is more z0'ward than z3'ward on the line traversing z0-z3

     and correspondingly for z2 */

  /* perp is distance from line, multiplied by dist z0-z3 */
  max_perp_sq = flatness * flatness * z3_0_dot;

  z1_perp = (y1 - y0) * x3_0 - (x1 - x0) * y3_0;
  if (z1_perp * z1_perp > max_perp_sq)
    goto subdivide;

  z2_perp = (y3 - y2) * x3_0 - (x3 - x2) * y3_0;
  if (z2_perp * z2_perp > max_perp_sq)
    goto subdivide;

  z1_dot = (x1 - x0) * x3_0 + (y1 - y0) * y3_0;
  if (z1_dot < 0 && z1_dot * z1_dot > max_perp_sq)
    goto subdivide;

  z2_dot = (x3 - x2) * x3_0 + (y3 - y2) * y3_0;
  if (z2_dot < 0 && z2_dot * z2_dot > max_perp_sq)
    goto subdivide;

  if (z1_dot + z1_dot > z3_0_dot)
    goto subdivide;

  if (z2_dot + z2_dot > z3_0_dot)
    goto subdivide;

 nosubdivide:
  /* don't subdivide */
  art_vpath_add_point (p_vpath, pn, pn_max,
		       ART_LINETO, x3, y3);
  return;

 subdivide:

  xa1 = (x0 + x1) * 0.5;
  ya1 = (y0 + y1) * 0.5;
  xa2 = (x0 + 2 * x1 + x2) * 0.25;
  ya2 = (y0 + 2 * y1 + y2) * 0.25;
  xb1 = (x1 + 2 * x2 + x3) * 0.25;
  yb1 = (y1 + 2 * y2 + y3) * 0.25;
  xb2 = (x2 + x3) * 0.5;
  yb2 = (y2 + y3) * 0.5;
  x_m = (xa2 + xb1) * 0.5;
  y_m = (ya2 + yb1) * 0.5;
#ifdef VERBOSE
  printf ("%g,%g %g,%g %g,%g %g,%g\n", xa1, ya1, xa2, ya2,
	  xb1, yb1, xb2, yb2);
#endif
  art_vpath_render_bez (p_vpath, pn, pn_max,
			x0, y0, xa1, ya1, xa2, ya2, x_m, y_m, flatness);
  art_vpath_render_bez (p_vpath, pn, pn_max,
			x_m, y_m, xb1, yb1, xb2, yb2, x3, y3, flatness);
}

/**
 * art_bez_path_to_vec: Create vpath from bezier path.
 * @bez: Bezier path.
 * @flatness: Flatness control.
 *
 * Creates a vector path closely approximating the bezier path defined by
 * @bez. The @flatness argument controls the amount of subdivision. In
 * general, the resulting vpath deviates by at most @flatness pixels
 * from the "ideal" path described by @bez.
 *
 * Return value: Newly allocated vpath.
 **/
ArtVpath *
art_bez_path_to_vec (const ArtBpath *bez, double flatness)
{
  ArtVpath *vec;
  int vec_n, vec_n_max;
  int bez_index;
  double x, y;

  vec_n = 0;
  vec_n_max = RENDER_SIZE;
  vec = art_new (ArtVpath, vec_n_max);

  /* Initialization is unnecessary because of the precondition that the
     bezier path does not begin with LINETO or CURVETO, but is here
     to make the code warning-free. */
  x = 0;
  y = 0;

  bez_index = 0;
  do
    {
#ifdef VERBOSE
      printf ("%s %g %g\n",
	      bez[bez_index].code == ART_CURVETO ? "curveto" :
	      bez[bez_index].code == ART_LINETO ? "lineto" :
	      bez[bez_index].code == ART_MOVETO ? "moveto" :
	      bez[bez_index].code == ART_MOVETO_OPEN ? "moveto-open" :
	      "end", bez[bez_index].x3, bez[bez_index].y3);
#endif
      /* make sure space for at least one more code */
      if (vec_n >= vec_n_max)
	art_expand (vec, ArtVpath, vec_n_max);
      switch (bez[bez_index].code)
	{
	case ART_MOVETO_OPEN:
	case ART_MOVETO:
	case ART_LINETO:
	  x = bez[bez_index].x3;
	  y = bez[bez_index].y3;
	  vec[vec_n].code = bez[bez_index].code;
	  vec[vec_n].x = x;
	  vec[vec_n].y = y;
	  vec_n++;
	  break;
	case ART_END:
	  vec[vec_n].code = bez[bez_index].code;
	  vec[vec_n].x = 0;
	  vec[vec_n].y = 0;
	  vec_n++;
	  break;
	case ART_CURVETO:
#ifdef VERBOSE
	  printf ("%g,%g %g,%g %g,%g %g,%g\n", x, y,
			 bez[bez_index].x1, bez[bez_index].y1,
			 bez[bez_index].x2, bez[bez_index].y2,
			 bez[bez_index].x3, bez[bez_index].y3);
#endif
	  art_vpath_render_bez (&vec, &vec_n, &vec_n_max,
				x, y,
				bez[bez_index].x1, bez[bez_index].y1,
				bez[bez_index].x2, bez[bez_index].y2,
				bez[bez_index].x3, bez[bez_index].y3,
				flatness);
	  x = bez[bez_index].x3;
	  y = bez[bez_index].y3;
	  break;
	}
    }
  while (bez[bez_index++].code != ART_END);
  return vec;
}

