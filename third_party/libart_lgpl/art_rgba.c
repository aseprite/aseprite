/*
 * art_rgba.c: Functions for manipulating RGBA pixel data.
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
 */

#include "config.h"
#include "art_misc.h"
#include "art_rgba.h"

#define ART_OPTIMIZE_SPACE

#ifndef ART_OPTIMIZE_SPACE
#include "art_rgba_table.c"
#endif

/**
 * art_rgba_rgba_composite: Composite RGBA image over RGBA buffer.
 * @dst: Destination RGBA buffer.
 * @src: Source RGBA buffer.
 * @n: Number of RGBA pixels to composite.
 *
 * Composites the RGBA pixels in @dst over the @src buffer.
 **/
void
art_rgba_rgba_composite (art_u8 *dst, const art_u8 *src, int n)
{
  int i;
#ifdef WORDS_BIGENDIAN
  art_u32 src_rgba, dst_rgba;
#else
  art_u32 src_abgr, dst_abgr;
#endif
  art_u8 src_alpha, dst_alpha;

  for (i = 0; i < n; i++)
    {
#ifdef WORDS_BIGENDIAN
      src_rgba = ((art_u32 *)src)[i];
      src_alpha = src_rgba & 0xff;
#else
      src_abgr = ((art_u32 *)src)[i];
      src_alpha = (src_abgr >> 24) & 0xff;
#endif
      if (src_alpha)
	{
	  if (src_alpha == 0xff ||
	      (
#ifdef WORDS_BIGENDIAN
	       dst_rgba = ((art_u32 *)dst)[i],
	       dst_alpha = dst_rgba & 0xff,
#else
	       dst_abgr = ((art_u32 *)dst)[i],
	       dst_alpha = (dst_abgr >> 24),
#endif
	       dst_alpha == 0))
#ifdef WORDS_BIGENDIAN
	    ((art_u32 *)dst)[i] = src_rgba;
#else
	    ((art_u32 *)dst)[i] = src_abgr;
#endif
	  else
	    {
	      int r, g, b, a;
	      int src_r, src_g, src_b;
	      int dst_r, dst_g, dst_b;
	      int tmp;
	      int c;

#ifdef ART_OPTIMIZE_SPACE
	      tmp = (255 - src_alpha) * (255 - dst_alpha) + 0x80;
	      a = 255 - ((tmp + (tmp >> 8)) >> 8);
	      c = ((src_alpha << 16) + (a >> 1)) / a;
#else
	      tmp = art_rgba_composite_table[(src_alpha << 8) + dst_alpha];
	      c = tmp & 0x1ffff;
	      a = tmp >> 24;
#endif
#ifdef WORDS_BIGENDIAN
	      src_r = (src_rgba >> 24) & 0xff;
	      src_g = (src_rgba >> 16) & 0xff;
	      src_b = (src_rgba >> 8) & 0xff;
	      dst_r = (dst_rgba >> 24) & 0xff;
	      dst_g = (dst_rgba >> 16) & 0xff;
	      dst_b = (dst_rgba >> 8) & 0xff;
#else
	      src_r = src_abgr & 0xff;
	      src_g = (src_abgr >> 8) & 0xff;
	      src_b = (src_abgr >> 16) & 0xff;
	      dst_r = dst_abgr & 0xff;
	      dst_g = (dst_abgr >> 8) & 0xff;
	      dst_b = (dst_abgr >> 16) & 0xff;
#endif
	      r = dst_r + (((src_r - dst_r) * c + 0x8000) >> 16);
	      g = dst_g + (((src_g - dst_g) * c + 0x8000) >> 16);
	      b = dst_b + (((src_b - dst_b) * c + 0x8000) >> 16);
#ifdef WORDS_BIGENDIAN
	    ((art_u32 *)dst)[i] = (r << 24) | (g << 16) | (b << 8) | a;
#else
	    ((art_u32 *)dst)[i] = (a << 24) | (b << 16) | (g << 8) | r;
#endif	      
	    }
	}
#if 0
      /* it's not clear to me this optimization really wins */
      else
	{
	  /* skip over run of transparent pixels */
	  for (; i < n - 1; i++)
	    {
#ifdef WORDS_BIGENDIAN
	      src_rgba = ((art_u32 *)src)[i + 1];
	      if (src_rgba & 0xff)
		break;
#else
	      src_abgr = ((art_u32 *)src)[i + 1];
	      if (src_abgr & 0xff000000)
		break;
#endif
	    }
	}
#endif
    }
}

/**
 * art_rgba_fill_run: fill an RGBA buffer a solid RGB color.
 * @buf: Buffer to fill.
 * @r: Red, range 0..255.
 * @g: Green, range 0..255.
 * @b: Blue, range 0..255.
 * @n: Number of RGB triples to fill.
 *
 * Fills a buffer with @n copies of the (@r, @g, @b) triple, solid
 * alpha. Thus, locations @buf (inclusive) through @buf + 4 * @n
 * (exclusive) are written.
 **/
void
art_rgba_fill_run (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int n)
{
  int i;
#ifdef WORDS_BIGENDIAN
  art_u32 src_rgba;
#else
  art_u32 src_abgr;
#endif

#ifdef WORDS_BIGENDIAN
  src_rgba = (r << 24) | (g << 16) | (b << 8) | 255;
#else
  src_abgr = (255 << 24) | (b << 16) | (g << 8) | r;
#endif
  for (i = 0; i < n; i++)
    {
#ifdef WORDS_BIGENDIAN
      ((art_u32 *)buf)[i] = src_rgba;
#else
      ((art_u32 *)buf)[i] = src_abgr;
#endif
    }    
}

/**
 * art_rgba_run_alpha: Render semitransparent color over RGBA buffer.
 * @buf: Buffer for rendering.
 * @r: Red, range 0..255.
 * @g: Green, range 0..255.
 * @b: Blue, range 0..255.
 * @alpha: Alpha, range 0..255.
 * @n: Number of RGB triples to render.
 *
 * Renders a sequential run of solid (@r, @g, @b) color over @buf with
 * opacity @alpha. Note that the range of @alpha is 0..255, in contrast
 * to art_rgb_run_alpha, which has a range of 0..256.
 **/
void
art_rgba_run_alpha (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int alpha, int n)
{
  int i;
#ifdef WORDS_BIGENDIAN
  art_u32 src_rgba, dst_rgba;
#else
  art_u32 src_abgr, dst_abgr;
#endif
  art_u8 dst_alpha;
  int a;
  int dst_r, dst_g, dst_b;
  int tmp;
  int c;

#ifdef WORDS_BIGENDIAN
  src_rgba = (r << 24) | (g << 16) | (b << 8) | alpha;
#else
  src_abgr = (alpha << 24) | (b << 16) | (g << 8) | r;
#endif
  for (i = 0; i < n; i++)
    {
#ifdef WORDS_BIGENDIAN
      dst_rgba = ((art_u32 *)buf)[i];
      dst_alpha = dst_rgba & 0xff;
#else
      dst_abgr = ((art_u32 *)buf)[i];
      dst_alpha = (dst_abgr >> 24) & 0xff;
#endif
      if (dst_alpha)
	{
#ifdef ART_OPTIMIZE_SPACE
	  tmp = (255 - alpha) * (255 - dst_alpha) + 0x80;
	  a = 255 - ((tmp + (tmp >> 8)) >> 8);
	  c = ((alpha << 16) + (a >> 1)) / a;
#else
	  tmp = art_rgba_composite_table[(alpha << 8) + dst_alpha];
	  c = tmp & 0x1ffff;
	  a = tmp >> 24;
#endif
#ifdef WORDS_BIGENDIAN
	  dst_r = (dst_rgba >> 24) & 0xff;
	  dst_g = (dst_rgba >> 16) & 0xff;
	  dst_b = (dst_rgba >> 8) & 0xff;
#else
	  dst_r = dst_abgr & 0xff;
	  dst_g = (dst_abgr >> 8) & 0xff;
	  dst_b = (dst_abgr >> 16) & 0xff;
#endif
	  dst_r += (((r - dst_r) * c + 0x8000) >> 16);
	  dst_g += (((g - dst_g) * c + 0x8000) >> 16);
	  dst_b += (((b - dst_b) * c + 0x8000) >> 16);
#ifdef WORDS_BIGENDIAN
	  ((art_u32 *)buf)[i] = (dst_r << 24) | (dst_g << 16) | (dst_b << 8) | a;
#else
	  ((art_u32 *)buf)[i] = (a << 24) | (dst_b << 16) | (dst_g << 8) | dst_r;
#endif
	}
      else
	{
#ifdef WORDS_BIGENDIAN
	  ((art_u32 *)buf)[i] = src_rgba;
#else
	  ((art_u32 *)buf)[i] = src_abgr;
#endif
	}
    }
}
