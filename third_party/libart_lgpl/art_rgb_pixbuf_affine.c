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
#include "art_pixbuf.h"
#include "art_rgb_affine.h"
#include "art_rgb_affine.h"
#include "art_rgb_rgba_affine.h"
#include "art_rgb_pixbuf_affine.h"

/* This module handles compositing of affine-transformed generic
   pixbuf images over rgb pixel buffers. */

/* Composite the source image over the destination image, applying the
   affine transform. */
/**
 * art_rgb_pixbuf_affine: Affine transform source RGB pixbuf and composite.
 * @dst: Destination image RGB buffer.
 * @x0: Left coordinate of destination rectangle.
 * @y0: Top coordinate of destination rectangle.
 * @x1: Right coordinate of destination rectangle.
 * @y1: Bottom coordinate of destination rectangle.
 * @dst_rowstride: Rowstride of @dst buffer.
 * @pixbuf: source image pixbuf.
 * @affine: Affine transform.
 * @level: Filter level.
 * @alphagamma: #ArtAlphaGamma for gamma-correcting the compositing.
 *
 * Affine transform the source image stored in @src, compositing over
 * the area of destination image @dst specified by the rectangle
 * (@x0, @y0) - (@x1, @y1). As usual in libart, the left and top edges
 * of this rectangle are included, and the right and bottom edges are
 * excluded.
 *
 * The @alphagamma parameter specifies that the alpha compositing be
 * done in a gamma-corrected color space. In the current
 * implementation, it is ignored.
 *
 * The @level parameter specifies the speed/quality tradeoff of the
 * image interpolation. Currently, only ART_FILTER_NEAREST is
 * implemented.
 **/
void
art_rgb_pixbuf_affine (art_u8 *dst,
		       int x0, int y0, int x1, int y1, int dst_rowstride,
		       const ArtPixBuf *pixbuf,
		       const double affine[6],
		       ArtFilterLevel level,
		       ArtAlphaGamma *alphagamma)
{
  if (pixbuf->format != ART_PIX_RGB)
    {
      art_warn ("art_rgb_pixbuf_affine: need RGB format image\n");
      return;
    }

  if (pixbuf->bits_per_sample != 8)
    {
      art_warn ("art_rgb_pixbuf_affine: need 8-bit sample data\n");
      return;
    }

  if (pixbuf->n_channels != 3 + (pixbuf->has_alpha != 0))
    {
      art_warn ("art_rgb_pixbuf_affine: need 8-bit sample data\n");
      return;
    }

  if (pixbuf->has_alpha)
    art_rgb_rgba_affine (dst, x0, y0, x1, y1, dst_rowstride,
			 pixbuf->pixels,
			 pixbuf->width, pixbuf->height, pixbuf->rowstride,
			 affine,
			 level,
			 alphagamma);
  else
    art_rgb_affine (dst, x0, y0, x1, y1, dst_rowstride,
		    pixbuf->pixels,
		    pixbuf->width, pixbuf->height, pixbuf->rowstride,
		    affine,
		    level,
		    alphagamma);
}
