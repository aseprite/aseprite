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

/* LGPL Copyright 1998 Raph Levien <raph@acm.org> */

#include <math.h>

#include "art_misc.h"
#include "art_vpath.h"
#include "art_uta.h"
#include "art_uta_vpath.h"

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

/**
 * art_uta_add_line: Add a line to the uta.
 * @uta: The uta to modify.
 * @x0: X coordinate of line start point.
 * @y0: Y coordinate of line start point.
 * @x1: X coordinate of line end point.
 * @y1: Y coordinate of line end point.
 * @rbuf: Buffer containing first difference of winding number.
 * @rbuf_rowstride: Rowstride of @rbuf.
 *
 * Add the line (@x0, @y0) - (@x1, @y1) to @uta, and also update the
 * winding number buffer used for rendering the interior. @rbuf
 * contains the first partial difference (in the X direction) of the
 * winding number, measured in grid cells. Thus, each time that a line
 * crosses a horizontal uta grid line, an entry of @rbuf is
 * incremented if @y1 > @y0, decremented otherwise.
 *
 * Note that edge handling is fairly delicate. Please rtfs for
 * details.
 **/
void
art_uta_add_line (ArtUta *uta, double x0, double y0, double x1, double y1,
		  int *rbuf, int rbuf_rowstride)
{
  int xmin, ymin;
  double xmax, ymax;
  int xmaxf, ymaxf;
  int xmaxc, ymaxc;
  int xt0, yt0;
  int xt1, yt1;
  int xf0, yf0;
  int xf1, yf1;
  int ix, ix1;
  ArtUtaBbox bb;

  xmin = floor (MIN(x0, x1));
  xmax = MAX(x0, x1);
  xmaxf = floor (xmax);
  xmaxc = ceil (xmax);
  ymin = floor (MIN(y0, y1));
  ymax = MAX(y0, y1);
  ymaxf = floor (ymax);
  ymaxc = ceil (ymax);
  xt0 = (xmin >> ART_UTILE_SHIFT) - uta->x0;
  yt0 = (ymin >> ART_UTILE_SHIFT) - uta->y0;
  xt1 = (xmaxf >> ART_UTILE_SHIFT) - uta->x0;
  yt1 = (ymaxf >> ART_UTILE_SHIFT) - uta->y0;
  if (xt0 == xt1 && yt0 == yt1)
    {
      /* entirely inside a microtile, this is easy! */
      xf0 = xmin & (ART_UTILE_SIZE - 1);
      yf0 = ymin & (ART_UTILE_SIZE - 1);
      xf1 = (xmaxf & (ART_UTILE_SIZE - 1)) + xmaxc - xmaxf;
      yf1 = (ymaxf & (ART_UTILE_SIZE - 1)) + ymaxc - ymaxf;

      ix = yt0 * uta->width + xt0;
      bb = uta->utiles[ix];
      if (bb == 0)
	bb = ART_UTA_BBOX_CONS(xf0, yf0, xf1, yf1);
      else
	bb = ART_UTA_BBOX_CONS(MIN(ART_UTA_BBOX_X0(bb), xf0),
			   MIN(ART_UTA_BBOX_Y0(bb), yf0),
			   MAX(ART_UTA_BBOX_X1(bb), xf1),
			   MAX(ART_UTA_BBOX_Y1(bb), yf1));
      uta->utiles[ix] = bb;
    }
  else
    {
      double dx, dy;
      int sx, sy;

      dx = x1 - x0;
      dy = y1 - y0;
      sx = dx > 0 ? 1 : dx < 0 ? -1 : 0;
      sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
      if (ymin == ymaxf)
	{
	  /* special case horizontal (dx/dy slope would be infinite) */
	  xf0 = xmin & (ART_UTILE_SIZE - 1);
	  yf0 = ymin & (ART_UTILE_SIZE - 1);
	  xf1 = (xmaxf & (ART_UTILE_SIZE - 1)) + xmaxc - xmaxf;
	  yf1 = (ymaxf & (ART_UTILE_SIZE - 1)) + ymaxc - ymaxf;

	  ix = yt0 * uta->width + xt0;
	  ix1 = yt0 * uta->width + xt1;
	  while (ix != ix1)
	    {
	      bb = uta->utiles[ix];
	      if (bb == 0)
		bb = ART_UTA_BBOX_CONS(xf0, yf0, ART_UTILE_SIZE, yf1);
	      else
		bb = ART_UTA_BBOX_CONS(MIN(ART_UTA_BBOX_X0(bb), xf0),
				   MIN(ART_UTA_BBOX_Y0(bb), yf0),
				   ART_UTILE_SIZE,
				   MAX(ART_UTA_BBOX_Y1(bb), yf1));
	      uta->utiles[ix] = bb;
	      xf0 = 0;
	      ix++;
	    }
	  bb = uta->utiles[ix];
	  if (bb == 0)
	    bb = ART_UTA_BBOX_CONS(0, yf0, xf1, yf1);
	  else
	    bb = ART_UTA_BBOX_CONS(0,
			       MIN(ART_UTA_BBOX_Y0(bb), yf0),
			       MAX(ART_UTA_BBOX_X1(bb), xf1),
			       MAX(ART_UTA_BBOX_Y1(bb), yf1));
	  uta->utiles[ix] = bb;
	}
      else
	{
	  /* Do a Bresenham-style traversal of the line */
	  double dx_dy;
	  double x, y;
	  double xn, yn;

	  /* normalize coordinates to uta origin */
	  x0 -= uta->x0 << ART_UTILE_SHIFT;
	  y0 -= uta->y0 << ART_UTILE_SHIFT;
	  x1 -= uta->x0 << ART_UTILE_SHIFT;
	  y1 -= uta->y0 << ART_UTILE_SHIFT;
	  if (dy < 0)
	    {
	      double tmp;

	      tmp = x0;
	      x0 = x1;
	      x1 = tmp;

	      tmp = y0;
	      y0 = y1;
	      y1 = tmp;

	      dx = -dx;
	      sx = -sx;
	      dy = -dy;
	      /* we leave sy alone, because it would always be 1,
		 and we need it for the rbuf stuff. */
	    }
	  xt0 = ((int)floor (x0) >> ART_UTILE_SHIFT);
	  xt1 = ((int)floor (x1) >> ART_UTILE_SHIFT);
	  /* now [xy]0 is above [xy]1 */

	  ix = yt0 * uta->width + xt0;
	  ix1 = yt1 * uta->width + xt1;
#ifdef VERBOSE
	  printf ("%% ix = %d,%d; ix1 = %d,%d\n", xt0, yt0, xt1, yt1);
#endif

	  dx_dy = dx / dy;
	  x = x0;
	  y = y0;
	  while (ix != ix1)
	    {
	      int dix;

	      /* figure out whether next crossing is horizontal or vertical */
#ifdef VERBOSE
	      printf ("%% %d,%d\n", xt0, yt0);
#endif
	      yn = (yt0 + 1) << ART_UTILE_SHIFT;
	      xn = x0 + dx_dy * (yn - y0);
	      if (xt0 != (int)floor (xn) >> ART_UTILE_SHIFT)
		{
		  /* horizontal crossing */
		  xt0 += sx;
		  dix = sx;
		  if (dx > 0)
		    {
		      xn = xt0 << ART_UTILE_SHIFT;
		      yn = y0 + (xn - x0) / dx_dy;

		      xf0 = (int)floor (x) & (ART_UTILE_SIZE - 1);
		      xf1 = ART_UTILE_SIZE;
		    }
		  else
		    {
		      xn = (xt0 + 1) << ART_UTILE_SHIFT;
		      yn = y0 + (xn - x0) / dx_dy;

		      xf0 = 0;
		      xmaxc = (int)ceil (x);
		      xf1 = xmaxc - ((xt0 + 1) << ART_UTILE_SHIFT);
		    }
		  ymaxf = (int)floor (yn);
		  ymaxc = (int)ceil (yn);
		  yf1 = (ymaxf & (ART_UTILE_SIZE - 1)) + ymaxc - ymaxf;
		}
	      else
		{
		  /* vertical crossing */
		  dix = uta->width;
		  xf0 = (int)floor (MIN(x, xn)) & (ART_UTILE_SIZE - 1);
		  xmax = MAX(x, xn);
		  xmaxc = (int)ceil (xmax);
		  xf1 = xmaxc - (xt0 << ART_UTILE_SHIFT);
		  yf1 = ART_UTILE_SIZE;

		  if (rbuf != NULL)
		    rbuf[yt0 * rbuf_rowstride + xt0] += sy;

		  yt0++;
		}
	      yf0 = (int)floor (y) & (ART_UTILE_SIZE - 1);
	      bb = uta->utiles[ix];
	      if (bb == 0)
		bb = ART_UTA_BBOX_CONS(xf0, yf0, xf1, yf1);
	      else
		bb = ART_UTA_BBOX_CONS(MIN(ART_UTA_BBOX_X0(bb), xf0),
				       MIN(ART_UTA_BBOX_Y0(bb), yf0),
				       MAX(ART_UTA_BBOX_X1(bb), xf1),
				       MAX(ART_UTA_BBOX_Y1(bb), yf1));
	      uta->utiles[ix] = bb;

	      x = xn;
	      y = yn;
	      ix += dix;
	    }
	  xmax = MAX(x, x1);
	  xmaxc = ceil (xmax);
	  ymaxc = ceil (y1);
	  xf0 = (int)floor (MIN(x1, x)) & (ART_UTILE_SIZE - 1);
	  yf0 = (int)floor (y) & (ART_UTILE_SIZE - 1);
	  xf1 = xmaxc - (xt0 << ART_UTILE_SHIFT);
	  yf1 = ymaxc - (yt0 << ART_UTILE_SHIFT);
	  bb = uta->utiles[ix];
	  if (bb == 0)
	    bb = ART_UTA_BBOX_CONS(xf0, yf0, xf1, yf1);
	  else
	    bb = ART_UTA_BBOX_CONS(MIN(ART_UTA_BBOX_X0(bb), xf0),
				   MIN(ART_UTA_BBOX_Y0(bb), yf0),
				   MAX(ART_UTA_BBOX_X1(bb), xf1),
				   MAX(ART_UTA_BBOX_Y1(bb), yf1));
	  uta->utiles[ix] = bb;
	}
    }
}

/**
 * art_uta_from_vpath: Generate uta covering a vpath.
 * @vec: The source vpath.
 *
 * Generates a uta covering @vec. The resulting uta is of course
 * approximate, ie it may cover more pixels than covered by @vec.
 *
 * Return value: the new uta.
 **/
ArtUta *
art_uta_from_vpath (const ArtVpath *vec)
{
  ArtUta *uta;
  ArtIRect bbox;
  int *rbuf;
  int i;
  double x, y;
  int sum;
  int xt, yt;
  ArtUtaBbox *utiles;
  ArtUtaBbox bb;
  int width;
  int height;
  int ix;

  art_vpath_bbox_irect (vec, &bbox);

  uta = art_uta_new_coords (bbox.x0, bbox.y0, bbox.x1, bbox.y1);

  width = uta->width;
  height = uta->height;
  utiles = uta->utiles;

  rbuf = art_new (int, width * height);
  for (i = 0; i < width * height; i++)
    rbuf[i] = 0;

  x = 0;
  y = 0;
  for (i = 0; vec[i].code != ART_END; i++)
    {
      switch (vec[i].code)
	{
	case ART_MOVETO:
	  x = vec[i].x;
	  y = vec[i].y;
	  break;
	case ART_LINETO:
	  art_uta_add_line (uta, vec[i].x, vec[i].y, x, y, rbuf, width);
	  x = vec[i].x;
	  y = vec[i].y;
	  break;
	default:
	  /* this shouldn't happen */
	  break;
	}
    }

  /* now add in the filling from rbuf */
  ix = 0;
  for (yt = 0; yt < height; yt++)
    {
      sum = 0;
      for (xt = 0; xt < width; xt++)
	{
	  sum += rbuf[ix];
	  /* Nonzero winding rule - others are possible, but hardly
	     worth it. */
	  if (sum != 0)
	    {
	      bb = utiles[ix];
	      bb &= 0xffff0000;
	      bb |= (ART_UTILE_SIZE << 8) | ART_UTILE_SIZE;
	      utiles[ix] = bb;
	      if (xt != width - 1)
		{
		  bb = utiles[ix + 1];
		  bb &= 0xffff00;
		  bb |= ART_UTILE_SIZE;
		  utiles[ix + 1] = bb;
		}
	      if (yt != height - 1)
		{
		  bb = utiles[ix + width];
		  bb &= 0xff0000ff;
		  bb |= ART_UTILE_SIZE << 8;
		  utiles[ix + width] = bb;
		  if (xt != width - 1)
		    {
		      utiles[ix + width + 1] &= 0xffff;
		    }
		}
	    }
	  ix++;
	}
    }

  art_free (rbuf);

  return uta;
}
