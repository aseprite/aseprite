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

/**
 * art_bpath_affine_transform: Affine transform an #ArtBpath.
 * @src: The source #ArtBpath.
 * @matrix: The affine transform.
 *
 * Affine transform the bezpath, returning a newly allocated #ArtBpath
 * (allocated using art_alloc()).
 *
 * Result (x', y') = (matrix[0] * x + matrix[2] * y + matrix[4],
 *                    matrix[1] * x + matrix[3] * y + matrix[5])
 *
 * Return value: the transformed #ArtBpath.
 **/
ArtBpath *
art_bpath_affine_transform (const ArtBpath *src, const double matrix[6])
{
  int i;
  int size;
  ArtBpath *new;
  ArtPathcode code;
  double x, y;

  for (i = 0; src[i].code != ART_END; i++);
  size = i;

  new = art_new (ArtBpath, size + 1);

  for (i = 0; i < size; i++)
    {
      code = src[i].code;
      new[i].code = code;
      if (code == ART_CURVETO)
	{
	  x = src[i].x1;
	  y = src[i].y1;
	  new[i].x1 = matrix[0] * x + matrix[2] * y + matrix[4];
	  new[i].y1 = matrix[1] * x + matrix[3] * y + matrix[5];
	  x = src[i].x2;
	  y = src[i].y2;
	  new[i].x2 = matrix[0] * x + matrix[2] * y + matrix[4];
	  new[i].y2 = matrix[1] * x + matrix[3] * y + matrix[5];
	}
      else
	{
	  new[i].x1 = 0;
	  new[i].y1 = 0;
	  new[i].x2 = 0;
	  new[i].y2 = 0;
	}
      x = src[i].x3;
      y = src[i].y3;
      new[i].x3 = matrix[0] * x + matrix[2] * y + matrix[4];
      new[i].y3 = matrix[1] * x + matrix[3] * y + matrix[5];
    }
  new[i].code = ART_END;
  new[i].x1 = 0;
  new[i].y1 = 0;
  new[i].x2 = 0;
  new[i].y2 = 0;
  new[i].x3 = 0;
  new[i].y3 = 0;

  return new;
}

