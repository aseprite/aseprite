/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "jinete/jlist.h"

#include "effect/colcurve.h"
#include "effect/effect.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

static struct {
  Curve *curve;
  int cmap[256];
} data;

CurvePoint *curve_point_new(int x, int y)
{
  CurvePoint *point;

  point = jnew(CurvePoint, 1);
  if (!point)
    return NULL;

  point->x = x;
  point->y = y;

  return point;
}

void curve_point_free(CurvePoint *point)
{
  jfree(point);
}

Curve *curve_new(int type)
{
  Curve *curve;

  curve = jnew(Curve, 1);
  if (!curve)
    return NULL;

  curve->type = type;
  curve->points = jlist_new();

  return curve;
}

void curve_free(Curve *curve)
{
  JLink link;

  JI_LIST_FOR_EACH(curve->points, link)
    curve_point_free(reinterpret_cast<CurvePoint*>(link->data));

  jlist_free(curve->points);
  jfree(curve);
}

void curve_add_point(Curve *curve, CurvePoint *point)
{
  JLink link;

  JI_LIST_FOR_EACH(curve->points, link)
    if (((CurvePoint *)link->data)->x >= point->x)
      break;

  jlist_insert_before(curve->points, link, point);
}

void curve_remove_point(Curve *curve, CurvePoint *point)
{
  jlist_remove(curve->points, point);
}

/**********************************************************************/
/* dacap: directly from GTK+ 2.2.2 source code */
/**********************************************************************/

/* Solve the tridiagonal equation system that determines the second
   derivatives for the interpolation points.  (Based on Numerical
   Recipies 2nd Edition.) */
static void
spline_solve(int n, float x[], float y[], float y2[])
{
  float p, sig;
  float* u;
  int i, k;

  u = (float*)jmalloc((n - 1) * sizeof(u[0]));

  y2[0] = u[0] = 0.0;   /* set lower boundary condition to "natural" */

  for (i = 1; i < n - 1; ++i)
    {
      sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
      p = sig * y2[i - 1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = ((y[i + 1] - y[i])
              / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]));
      u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }

  y2[n - 1] = 0.0;
  for (k = n - 2; k >= 0; --k)
    y2[k] = y2[k] * y2[k + 1] + u[k];

  jfree(u);
}

static float
spline_eval(int n, float x[], float y[], float y2[], float val)
{
  int k_lo, k_hi, k;
  float h, b, a;

  /* do a binary search for the right interval: */
  k_lo = 0; k_hi = n - 1;
  while (k_hi - k_lo > 1)
    {
      k = (k_hi + k_lo) / 2;
      if (x[k] > val)
        k_hi = k;
      else
        k_lo = k;
    }

  h = x[k_hi] - x[k_lo];
  /* TODO */
  /* ASSERT(h > 0.0); */

  a = (x[k_hi] - val) / h;
  b = (val - x[k_lo]) / h;
  return a*y[k_lo] + b*y[k_hi] +
    ((a*a*a - a)*y2[k_lo] + (b*b*b - b)*y2[k_hi]) * (h*h)/6.0;
}

void curve_get_values(Curve *curve, int x1, int x2, int *values)
{
  int x, num_points = jlist_length(curve->points);

  if (num_points == 0) {
    for (x=x1; x<=x2; x++)
      values[x-x1] = 0;
  }
  else if (num_points == 1) {
    for (x=x1; x<=x2; x++)
      values[x-x1] = ((CurvePoint *)jlist_first_data(curve->points))->y;
  }
  else {
    switch (curve->type) {

      case CURVE_LINEAR: {
	JLink link, last = jlist_last(curve->points);
	CurvePoint *p, *n;

	JI_LIST_FOR_EACH(curve->points, link)
	  if (((CurvePoint *)link->data)->x >= x1)
	    break;

	for (x=x1; x<=x2; x++) {
	  while ((link != curve->points->end) &&
		 (x > ((CurvePoint *)link->data)->x))
	    link = link->next;

	  if (link != curve->points->end) {
	    if (link->prev != curve->points->end) {
	      p = reinterpret_cast<CurvePoint*>(link->prev->data);
	      n = reinterpret_cast<CurvePoint*>(link->data);

	      values[x-x1] = p->y + (n->y-p->y) * (x-p->x) / (n->x-p->x);
	    }
	    else {
	      values[x-x1] = ((CurvePoint *)link->data)->y;
	    }
	  }
	  else {
	    values[x-x1] = ((CurvePoint *)last->data)->y;
	  }
	}
	break;
      }

      case CURVE_SPLINE: {
	/* dacap: almost all code from GTK+ 2.2.2 source code */
	float rx, ry, dx, min_x, *mem, *xv, *yv, *y2v, prev;
	int dst, veclen = x2-x1+1;
	JLink link;

	min_x = 0;

	mem = (float*)jmalloc(3 * num_points * sizeof(float));
	xv  = mem;
	yv  = mem + num_points;
	y2v = mem + 2*num_points;

	prev = min_x - 1.0;
	dst = 0;
	JI_LIST_FOR_EACH(curve->points, link) {
	  if (((CurvePoint *)link->data)->x > prev) {
	    prev    = ((CurvePoint *)link->data)->x;
	    xv[dst] = ((CurvePoint *)link->data)->x;
	    yv[dst] = ((CurvePoint *)link->data)->y;
	    ++dst;
	  }
	}

	spline_solve(dst, xv, yv, y2v);

	rx = min_x;
	dx = (x2 - x1) / (veclen-1);
	for (x=0; x<veclen; ++x, rx+=dx) {
	  ry = spline_eval(dst, xv, yv, y2v, rx);
#if 0
/* 	  if (ry < curve->min_y) ry = curve->min_y; */
/* 	  if (ry > curve->max_y) ry = curve->max_y; */
#else
/* 	  if (ry < 0) ry = 0; */
/* 	  else if (ry > 255) ry = 255; */
#endif
	  values[x] = ry;
	}

	jfree(mem);
	break;
      }
    }
  }
}

void set_color_curve(Curve *curve)
{
  int c;

  data.curve = curve;

  /* generate the color convertion map */
  curve_get_values(data.curve, 0, 255, data.cmap);

  for (c=0; c<256; c++)
    data.cmap[c] = MID(0, data.cmap[c], 255);
}

void apply_color_curve4(Effect *effect)
{
  ase_uint32 *src_address;
  ase_uint32 *dst_address;
  int x, c, r, g, b, a;

  src_address = ((ase_uint32 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint32 **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    r = _rgba_getr(c);
    g = _rgba_getg(c);
    b = _rgba_getb(c);
    a = _rgba_geta(c);

    if (effect->target & TARGET_RED_CHANNEL) r = data.cmap[r];
    if (effect->target & TARGET_GREEN_CHANNEL) g = data.cmap[g];
    if (effect->target & TARGET_BLUE_CHANNEL) b = data.cmap[b];
    if (effect->target & TARGET_ALPHA_CHANNEL) a = data.cmap[a];

    *(dst_address++) = _rgba(r, g, b, a);
  }
}

void apply_color_curve2 (Effect *effect)
{
  ase_uint16 *src_address;
  ase_uint16 *dst_address;
  int x, c, k, a;

  src_address = ((ase_uint16 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint16 **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    k = _graya_getv(c);
    a = _graya_geta(c);

    if (effect->target & TARGET_GRAY_CHANNEL) k = data.cmap[k];
    if (effect->target & TARGET_ALPHA_CHANNEL) a = data.cmap[a];

    *(dst_address++) = _graya(k, a);
  }
}

void apply_color_curve1(Effect *effect)
{
  Palette *pal = get_current_palette();
  RgbMap* rgbmap = effect->sprite->getRgbMap();
  ase_uint8* src_address;
  ase_uint8* dst_address;
  int x, c, r, g, b;

  src_address = ((ase_uint8**)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint8**)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    if (effect->target & TARGET_INDEX_CHANNEL) {
      c = data.cmap[c];
    }
    else {
      r = _rgba_getr(pal->getEntry(c));
      g = _rgba_getg(pal->getEntry(c));
      b = _rgba_getb(pal->getEntry(c));

      if (effect->target & TARGET_RED_CHANNEL) r = data.cmap[r];
      if (effect->target & TARGET_GREEN_CHANNEL) g = data.cmap[g];
      if (effect->target & TARGET_BLUE_CHANNEL) b = data.cmap[b];

      c = rgbmap->mapColor(r, g, b);
    }

    *(dst_address++) = MID(0, c, pal->size()-1);
  }
}
