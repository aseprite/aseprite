/*
 * art_render_gradient.h: Gradient image source for modular rendering.
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

#ifndef __ART_RENDER_GRADIENT_H__
#define __ART_RENDER_GRADIENT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ArtGradientLinear ArtGradientLinear;
typedef struct _ArtGradientRadial ArtGradientRadial;
typedef struct _ArtGradientStop ArtGradientStop;

typedef enum {
  ART_GRADIENT_PAD,
  ART_GRADIENT_REFLECT,
  ART_GRADIENT_REPEAT
} ArtGradientSpread;

struct _ArtGradientLinear {
  double a;
  double b;
  double c;
  ArtGradientSpread spread;
  int n_stops;
  ArtGradientStop *stops;
};

struct _ArtGradientRadial {
  double affine[6]; /* transforms user coordinates to unit circle */
  double fx, fy;    /* focal point in unit circle coords */
  int n_stops;
  ArtGradientStop *stops;
};

struct _ArtGradientStop {
  double offset;
  ArtPixMaxDepth color[ART_MAX_CHAN + 1];
};

void
art_render_gradient_linear (ArtRender *render,
			    const ArtGradientLinear *gradient,
			    ArtFilterLevel level);

void
art_render_gradient_radial (ArtRender *render,
			    const ArtGradientRadial *gradient,
			    ArtFilterLevel level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_RENDER_GRADIENT_H__ */
