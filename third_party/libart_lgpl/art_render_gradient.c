/*
 * art_render_gradient.c: Gradient image source for modular rendering.
 *
 * Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 2000 Raph Levien
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
 *
 * Authors: Raph Levien <raph@acm.org>
 *          Alexander Larsson <alla@lysator.liu.se>
 */

#include <math.h>

#include "art_misc.h"
#include "art_alphagamma.h"
#include "art_filterlevel.h"

#include "art_render.h"
#include "art_render_gradient.h"

typedef struct _ArtImageSourceGradLin ArtImageSourceGradLin;
typedef struct _ArtImageSourceGradRad ArtImageSourceGradRad;

struct _ArtImageSourceGradLin {
  ArtImageSource super;
  const ArtGradientLinear *gradient;
};

struct _ArtImageSourceGradRad {
  ArtImageSource super;
  const ArtGradientRadial *gradient;
  double a;
};

#define EPSILON 1e-6

/**
 * art_render_gradient_setpix: Set a gradient pixel.
 * @render: The render object.
 * @dst: Pointer to destination (where to store pixel).
 * @n_stops: Number of stops in @stops.
 * @stops: The stops for the gradient.
 * @offset: The offset.
 *
 * @n_stops must be > 0.
 *
 * Sets a gradient pixel, storing it at @dst.
 **/
static void
art_render_gradient_setpix (ArtRender *render,
			    art_u8 *dst,
			    int n_stops, ArtGradientStop *stops,
			    double offset)
{
  int ix;
  int j;
  double off0, off1;
  int n_ch = render->n_chan + 1;

  for (ix = 0; ix < n_stops; ix++)
    if (stops[ix].offset > offset)
      break;
  /* stops[ix - 1].offset < offset < stops[ix].offset */
  if (ix > 0 && ix < n_stops)
    {
      off0 = stops[ix - 1].offset;
      off1 = stops[ix].offset;
      if (fabs (off1 - off0) > EPSILON)
	{
	  double interp;

	  interp = (offset - off0) / (off1 - off0);
	  for (j = 0; j < n_ch; j++)
	    {
	      int z0, z1;
	      int z;
	      z0 = stops[ix - 1].color[j];
	      z1 = stops[ix].color[j];
	      z = floor (z0 + (z1 - z0) * interp + 0.5);
	      if (render->buf_depth == 8)
		dst[j] = ART_PIX_8_FROM_MAX (z);
	      else /* (render->buf_depth == 16) */
		((art_u16 *)dst)[j] = z;
	    }
	  return;
	}
    }
  else if (ix == n_stops)
    ix--;

  for (j = 0; j < n_ch; j++)
    {
      int z;
      z = stops[ix].color[j];
      if (render->buf_depth == 8)
	dst[j] = ART_PIX_8_FROM_MAX (z);
      else /* (render->buf_depth == 16) */
	((art_u16 *)dst)[j] = z;
    }
}

static void
art_render_gradient_linear_done (ArtRenderCallback *self, ArtRender *render)
{
  art_free (self);
}

static void
art_render_gradient_linear_render (ArtRenderCallback *self, ArtRender *render,
				   art_u8 *dest, int y)
{
  ArtImageSourceGradLin *z = (ArtImageSourceGradLin *)self;
  const ArtGradientLinear *gradient = z->gradient;
  int pixstride = (render->n_chan + 1) * (render->depth >> 3);
  int x;
  int width = render->x1 - render->x0;
  double offset, d_offset;
  double actual_offset;
  int n_stops = gradient->n_stops;
  ArtGradientStop *stops = gradient->stops;
  art_u8 *bufp = render->image_buf;
  ArtGradientSpread spread = gradient->spread;

  offset = render->x0 * gradient->a + y * gradient->b + gradient->c;
  d_offset = gradient->a;

  for (x = 0; x < width; x++)
    {
      if (spread == ART_GRADIENT_PAD)
	actual_offset = offset;
      else if (spread == ART_GRADIENT_REPEAT)
	actual_offset = offset - floor (offset);
      else /* (spread == ART_GRADIENT_REFLECT) */
	{
	  double tmp;

	  tmp = offset - 2 * floor (0.5 * offset);
	  actual_offset = tmp > 1 ? 2 - tmp : tmp;
	}
      art_render_gradient_setpix (render, bufp, n_stops, stops, actual_offset);
      offset += d_offset;
      bufp += pixstride;
    }
}

static void
art_render_gradient_linear_negotiate (ArtImageSource *self, ArtRender *render,
				      ArtImageSourceFlags *p_flags,
				      int *p_buf_depth, ArtAlphaType *p_alpha)
{
  self->super.render = art_render_gradient_linear_render;
  *p_flags = 0;
  *p_buf_depth = render->depth;
  *p_alpha = ART_ALPHA_PREMUL;
}

/**
 * art_render_gradient_linear: Add a linear gradient image source.
 * @render: The render object.
 * @gradient: The linear gradient.
 *
 * Adds the linear gradient @gradient as the image source for rendering
 * in the render object @render.
 **/
void
art_render_gradient_linear (ArtRender *render,
			    const ArtGradientLinear *gradient,
			    ArtFilterLevel level)
{
  ArtImageSourceGradLin *image_source = art_new (ArtImageSourceGradLin, 1);

  image_source->super.super.render = NULL;
  image_source->super.super.done = art_render_gradient_linear_done;
  image_source->super.negotiate = art_render_gradient_linear_negotiate;

  image_source->gradient = gradient;

  art_render_add_image_source (render, &image_source->super);
}

static void
art_render_gradient_radial_done (ArtRenderCallback *self, ArtRender *render)
{
  art_free (self);
}

static void
art_render_gradient_radial_render (ArtRenderCallback *self, ArtRender *render,
				   art_u8 *dest, int y)
{
  ArtImageSourceGradRad *z = (ArtImageSourceGradRad *)self;
  const ArtGradientRadial *gradient = z->gradient;
  int pixstride = (render->n_chan + 1) * (render->depth >> 3);
  int x;
  int x0 = render->x0;
  int width = render->x1 - x0;
  int n_stops = gradient->n_stops;
  ArtGradientStop *stops = gradient->stops;
  art_u8 *bufp = render->image_buf;
  double fx = gradient->fx;
  double fy = gradient->fy;
  double dx, dy;
  double *affine = gradient->affine;
  double aff0 = affine[0];
  double aff1 = affine[1];
  const double a = z->a;
  const double arecip = 1.0 / a;
  double b, db;
  double c, dc, ddc;
  double b_a, db_a;
  double rad, drad, ddrad;

  dx = x0 * aff0 + y * affine[2] + affine[4] - fx;
  dy = x0 * aff1 + y * affine[3] + affine[5] - fy;
  b = dx * fx + dy * fy;
  db = aff0 * fx + aff1 * fy;
  c = dx * dx + dy * dy;
  dc = 2 * aff0 * dx + aff0 * aff0 + 2 * aff1 * dy + aff1 * aff1;
  ddc = 2 * aff0 * aff0 + 2 * aff1 * aff1;

  b_a = b * arecip;
  db_a = db * arecip;

  rad = b_a * b_a + c * arecip;
  drad = 2 * b_a * db_a + db_a * db_a + dc * arecip;
  ddrad = 2 * db_a * db_a + ddc * arecip;

  for (x = 0; x < width; x++)
    {
      double z;

      if (rad > 0)
	z = b_a + sqrt (rad);
      else
	z = b_a;
      art_render_gradient_setpix (render, bufp, n_stops, stops, z);
      bufp += pixstride;
      b_a += db_a;
      rad += drad;
      drad += ddrad;
    }
}

static void
art_render_gradient_radial_negotiate (ArtImageSource *self, ArtRender *render,
				      ArtImageSourceFlags *p_flags,
				      int *p_buf_depth, ArtAlphaType *p_alpha)
{
  self->super.render = art_render_gradient_radial_render;
  *p_flags = 0;
  *p_buf_depth = render->depth;
  *p_alpha = ART_ALPHA_PREMUL;
}

/**
 * art_render_gradient_radial: Add a radial gradient image source.
 * @render: The render object.
 * @gradient: The radial gradient.
 *
 * Adds the radial gradient @gradient as the image source for rendering
 * in the render object @render.
 **/
void
art_render_gradient_radial (ArtRender *render,
			    const ArtGradientRadial *gradient,
			    ArtFilterLevel level)
{
  ArtImageSourceGradRad *image_source = art_new (ArtImageSourceGradRad, 1);
  double fx = gradient->fx;
  double fy = gradient->fy;

  image_source->super.super.render = NULL;
  image_source->super.super.done = art_render_gradient_radial_done;
  image_source->super.negotiate = art_render_gradient_radial_negotiate;

  image_source->gradient = gradient;
  /* todo: sanitycheck fx, fy? */
  image_source->a = 1 - fx * fx - fy * fy;

  art_render_add_image_source (render, &image_source->super);
}
