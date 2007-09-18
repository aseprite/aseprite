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
#include "art_rgb_bitmap_affine.h"

/* This module handles compositing of affine-transformed bitmap images
   over rgb pixel buffers. */

/* Composite the source image over the destination image, applying the
   affine transform. Foreground color is given and assumed to be
   opaque, background color is assumed to be fully transparent. */

static void
art_rgb_bitmap_affine_opaque (art_u8 *dst,
			      int x0, int y0, int x1, int y1,
			      int dst_rowstride,
			      const art_u8 *src,
			      int src_width, int src_height, int src_rowstride,
			      art_u32 rgb,
			      const double affine[6],
			      ArtFilterLevel level,
			      ArtAlphaGamma *alphagamma)
{
  /* Note: this is a slow implementation, and is missing all filter
     levels other than NEAREST. It is here for clarity of presentation
     and to establish the interface. */
  int x, y;
  double inv[6];
  art_u8 *dst_p, *dst_linestart;
  const art_u8 *src_p;
  ArtPoint pt, src_pt;
  int src_x, src_y;
  art_u8 r, g, b;
  int run_x0, run_x1;

  r = rgb >> 16;
  g = (rgb >> 8) & 0xff;
  b = rgb & 0xff;
  dst_linestart = dst;
  art_affine_invert (inv, affine);
  for (y = y0; y < y1; y++)
    {
      pt.y = y + 0.5;
      run_x0 = x0;
      run_x1 = x1;
      art_rgb_affine_run (&run_x0, &run_x1, y, src_width, src_height,
			  inv);
      dst_p = dst_linestart + (run_x0 - x0) * 3;
      for (x = run_x0; x < run_x1; x++)
	{
	  pt.x = x + 0.5;
	  art_affine_point (&src_pt, &pt, inv);
	  src_x = floor (src_pt.x);
	  src_y = floor (src_pt.y);
	  src_p = src + (src_y * src_rowstride) + (src_x >> 3);
	  if (*src_p & (128 >> (src_x & 7)))
	    {
	      dst_p[0] = r;
	      dst_p[1] = g;
	      dst_p[2] = b;
	    }
	  dst_p += 3;
	}
      dst_linestart += dst_rowstride;
    }
}
/* Composite the source image over the destination image, applying the
   affine transform. Foreground color is given, background color is
   assumed to be fully transparent. */

/**
 * art_rgb_bitmap_affine: Affine transform source bitmap image and composite.
 * @dst: Destination image RGB buffer.
 * @x0: Left coordinate of destination rectangle.
 * @y0: Top coordinate of destination rectangle.
 * @x1: Right coordinate of destination rectangle.
 * @y1: Bottom coordinate of destination rectangle.
 * @dst_rowstride: Rowstride of @dst buffer.
 * @src: Source image bitmap buffer.
 * @src_width: Width of source image.
 * @src_height: Height of source image.
 * @src_rowstride: Rowstride of @src buffer.
 * @rgba: RGBA foreground color, in 0xRRGGBBAA.
 * @affine: Affine transform.
 * @level: Filter level.
 * @alphagamma: #ArtAlphaGamma for gamma-correcting the compositing.
 *
 * Affine transform the source image stored in @src, compositing over
 * the area of destination image @dst specified by the rectangle
 * (@x0, @y0) - (@x1, @y1).
 *
 * The source bitmap stored with MSB as the leftmost pixel. Source 1
 * bits correspond to the semitransparent color @rgba, while source 0
 * bits are transparent.
 *
 * See art_rgb_affine() for a description of additional parameters.
 **/
void
art_rgb_bitmap_affine (art_u8 *dst,
		       int x0, int y0, int x1, int y1, int dst_rowstride,
		       const art_u8 *src,
		       int src_width, int src_height, int src_rowstride,
		       art_u32 rgba,
		       const double affine[6],
		       ArtFilterLevel level,
		       ArtAlphaGamma *alphagamma)
{
  /* Note: this is a slow implementation, and is missing all filter
     levels other than NEAREST. It is here for clarity of presentation
     and to establish the interface. */
  int x, y;
  double inv[6];
  art_u8 *dst_p, *dst_linestart;
  const art_u8 *src_p;
  ArtPoint pt, src_pt;
  int src_x, src_y;
  int alpha;
  art_u8 bg_r, bg_g, bg_b;
  art_u8 fg_r, fg_g, fg_b;
  art_u8 r, g, b;
  int run_x0, run_x1;

  alpha = rgba & 0xff;
  if (alpha == 0xff)
    {
      art_rgb_bitmap_affine_opaque (dst, x0, y0, x1, y1, dst_rowstride,
				    src,
				    src_width, src_height, src_rowstride,
				    rgba >> 8,
				    affine,
				    level,
				    alphagamma);
      return;
    }
  /* alpha = (65536 * alpha) / 255; */
  alpha = (alpha << 8) + alpha + (alpha >> 7);
  r = rgba >> 24;
  g = (rgba >> 16) & 0xff;
  b = (rgba >> 8) & 0xff;
  dst_linestart = dst;
  art_affine_invert (inv, affine);
  for (y = y0; y < y1; y++)
    {
      pt.y = y + 0.5;
      run_x0 = x0;
      run_x1 = x1;
      art_rgb_affine_run (&run_x0, &run_x1, y, src_width, src_height,
			  inv);
      dst_p = dst_linestart + (run_x0 - x0) * 3;
      for (x = run_x0; x < run_x1; x++)
	{
	  pt.x = x + 0.5;
	  art_affine_point (&src_pt, &pt, inv);
	  src_x = floor (src_pt.x);
	  src_y = floor (src_pt.y);
	  src_p = src + (src_y * src_rowstride) + (src_x >> 3);
	  if (*src_p & (128 >> (src_x & 7)))
	    {
	      bg_r = dst_p[0];
	      bg_g = dst_p[1];
	      bg_b = dst_p[2];

	      fg_r = bg_r + (((r - bg_r) * alpha + 0x8000) >> 16);
	      fg_g = bg_g + (((g - bg_g) * alpha + 0x8000) >> 16);
	      fg_b = bg_b + (((b - bg_b) * alpha + 0x8000) >> 16);

	      dst_p[0] = fg_r;
	      dst_p[1] = fg_g;
	      dst_p[2] = fg_b;
	    }
	  dst_p += 3;
	}
      dst_linestart += dst_rowstride;
    }
}
