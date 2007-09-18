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

#include <math.h>
#include "art_misc.h"
#include "art_point.h"
#include "art_affine.h"
#include "art_rgb_affine_private.h"

/* Private functions for the rgb affine image compositors - primarily,
   the determination of runs, eliminating the need for source image
   bbox calculation in the inner loop. */

/* Determine a "run", such that the inverse affine of all pixels from
   (x0, y) inclusive to (x1, y) exclusive fit within the bounds
   of the source image.

   Initial values of x0, x1, and result values stored in first two
   pointer arguments.
*/

#define EPSILON 1e-6

void
art_rgb_affine_run (int *p_x0, int *p_x1, int y,
		    int src_width, int src_height,
		    const double affine[6])
{
  int x0, x1;
  double z;
  double x_intercept;
  int xi;

  x0 = *p_x0;
  x1 = *p_x1;

  /* do left and right edges */
  if (affine[0] > EPSILON)
    {
      z = affine[2] * (y + 0.5) + affine[4];
      x_intercept = -z / affine[0];
      xi = ceil (x_intercept + EPSILON - 0.5);
      if (xi > x0)
	x0 = xi;
      x_intercept = (-z + src_width) / affine[0];
      xi = ceil (x_intercept - EPSILON - 0.5);
      if (xi < x1)
	x1 = xi;
    }
  else if (affine[0] < -EPSILON)
    {
      z = affine[2] * (y + 0.5) + affine[4];
      x_intercept = (-z + src_width) / affine[0];
      xi = ceil (x_intercept + EPSILON - 0.5);
      if (xi > x0)
	x0 = xi;
      x_intercept = -z / affine[0];
      xi = ceil (x_intercept - EPSILON - 0.5);
      if (xi < x1)
	x1 = xi;
    }
  else
    {
      z = affine[2] * (y + 0.5) + affine[4];
      if (z < 0 || z >= src_width)
	{
	  *p_x1 = *p_x0;
	  return;
	}
    }

  /* do top and bottom edges */
  if (affine[1] > EPSILON)
    {
      z = affine[3] * (y + 0.5) + affine[5];
      x_intercept = -z / affine[1];
      xi = ceil (x_intercept + EPSILON - 0.5);
      if (xi > x0)
	x0 = xi;
      x_intercept = (-z + src_height) / affine[1];
      xi = ceil (x_intercept - EPSILON - 0.5);
      if (xi < x1)
	x1 = xi;
    }
  else if (affine[1] < -EPSILON)
    {
      z = affine[3] * (y + 0.5) + affine[5];
      x_intercept = (-z + src_height) / affine[1];
      xi = ceil (x_intercept + EPSILON - 0.5);
      if (xi > x0)
	x0 = xi;
      x_intercept = -z / affine[1];
      xi = ceil (x_intercept - EPSILON - 0.5);
      if (xi < x1)
	x1 = xi;
    }
  else
    {
      z = affine[3] * (y + 0.5) + affine[5];
      if (z < 0 || z >= src_height)
	{
	  *p_x1 = *p_x0;
	  return;
	}
    }

  *p_x0 = x0;
  *p_x1 = x1;
}
