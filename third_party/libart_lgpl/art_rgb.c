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

#include <string.h>	/* for memset */
#include "art_misc.h"
#include "art_rgb.h"

#include "config.h" /* for endianness */

/* Basic operators for manipulating 24-bit packed RGB buffers. */

#define COLOR_RUN_COMPLEX

#ifdef COLOR_RUN_SIMPLE
/* This is really slow. Is there any way we might speed it up?
   Two ideas:

   First, maybe we should be working at 32-bit alignment. Then,
   this can be a simple loop over word stores.

   Second, we can keep working at 24-bit alignment, but have some
   intelligence about storing. For example, we can iterate over
   4-pixel chunks (aligned at 4 pixels), with an inner loop
   something like:

   *buf++ = v1;
   *buf++ = v2;
   *buf++ = v3;

   One source of extra complexity is the need to make sure linebuf is
   aligned to a 32-bit boundary.

   This second alternative has some complexity to it, but is
   appealing because it really minimizes the memory bandwidth. */
void
art_rgb_fill_run (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, gint n)
{
  int i;

  if (r == g && g == b)
    {
      memset (buf, g, n + n + n);
    }
  else
    {
      for (i = 0; i < n; i++)
	{
	  *buf++ = r;
	  *buf++ = g;
	  *buf++ = b;
	}
    }
}
#endif

#ifdef COLOR_RUN_COMPLEX
/* This implements the second of the two ideas above. The test results
   are _very_ encouraging - it seems the speed is within 10% of
   memset, which is quite good! */
/**
 * art_rgb_fill_run: fill a buffer a solid RGB color.
 * @buf: Buffer to fill.
 * @r: Red, range 0..255.
 * @g: Green, range 0..255.
 * @b: Blue, range 0..255.
 * @n: Number of RGB triples to fill.
 *
 * Fills a buffer with @n copies of the (@r, @g, @b) triple. Thus,
 * locations @buf (inclusive) through @buf + 3 * @n (exclusive) are
 * written.
 *
 * The implementation of this routine is very highly optimized.
 **/
void
art_rgb_fill_run (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int n)
{
  int i;
  unsigned int v1, v2, v3;

  if (r == g && g == b)
    {
      memset (buf, g, n + n + n);
    }
  else
    {
      if (n < 8)
	{
	  for (i = 0; i < n; i++)
	    {
	      *buf++ = r;
	      *buf++ = g;
	      *buf++ = b;
	    }
	} else {
	  /* handle prefix up to byte alignment */
	  /* I'm worried about this cast on sizeof(long) != sizeof(uchar *)
	     architectures, but it _should_ work. */
	  for (i = 0; ((unsigned long)buf) & 3; i++)
	    {
	      *buf++ = r;
	      *buf++ = g;
	      *buf++ = b;
	    }
#ifndef WORDS_BIGENDIAN
	  v1 = r | (g << 8) | (b << 16) | (r << 24);
	  v3 = (v1 << 8) | b;
	  v2 = (v3 << 8) | g;
#else
	  v1 = (r << 24) | (g << 16) | (b << 8) | r;
	  v2 = (v1 << 8) | g;
	  v3 = (v2 << 8) | b;
#endif
	  for (; i < n - 3; i += 4)
	    {
	      ((art_u32 *)buf)[0] = v1;
	      ((art_u32 *)buf)[1] = v2;
	      ((art_u32 *)buf)[2] = v3;
	      buf += 12;
	    }
	  /* handle postfix */
	  for (; i < n; i++)
	    {
	      *buf++ = r;
	      *buf++ = g;
	      *buf++ = b;
	    }
	}
    }
}
#endif

/**
 * art_rgb_run_alpha: Render semitransparent color over RGB buffer.
 * @buf: Buffer for rendering.
 * @r: Red, range 0..255.
 * @g: Green, range 0..255.
 * @b: Blue, range 0..255.
 * @alpha: Alpha, range 0..256.
 * @n: Number of RGB triples to render.
 *
 * Renders a sequential run of solid (@r, @g, @b) color over @buf with
 * opacity @alpha.
 **/
void
art_rgb_run_alpha (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int alpha, int n)
{
  int i;
  int v;

  for (i = 0; i < n; i++)
    {
      v = *buf;
      *buf++ = v + (((r - v) * alpha + 0x80) >> 8);
      v = *buf;
      *buf++ = v + (((g - v) * alpha + 0x80) >> 8);
      v = *buf;
      *buf++ = v + (((b - v) * alpha + 0x80) >> 8);
    }
}

