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

/* Render a sorted vector path into a graymap. */

#include <string.h>	/* for memset */
#include "art_misc.h"

#include "art_svp.h"
#include "art_svp_render_aa.h"
#include "art_gray_svp.h"

typedef struct _ArtGraySVPData ArtGraySVPData;

struct _ArtGraySVPData {
  art_u8 *buf;
  int rowstride;
  int x0, x1;
};

static void
art_gray_svp_callback (void *callback_data, int y,
		       int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtGraySVPData *data = (ArtGraySVPData *)callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  int running_sum = start;
  int x0, x1;
  int k;

#if 0
  printf ("start = %d", start);
  running_sum = start;
  for (k = 0; k < n_steps; k++)
    {
      running_sum += steps[k].delta;
      printf (" %d:%d", steps[k].x, running_sum >> 16);
    }
  printf ("\n");
#endif

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	memset (linebuf, running_sum >> 16, run_x1 - x0);

      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    memset (linebuf + run_x0 - x0, running_sum >> 16, run_x1 - run_x0);
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	memset (linebuf + run_x1 - x0, running_sum >> 16, x1 - run_x1);
    }
  else
    {
      memset (linebuf, running_sum >> 16, x1 - x0);
    }

  data->buf += data->rowstride;
}

/**
 * art_gray_svp_aa: Render the vector path into the bytemap.
 * @svp: The SVP to render.
 * @x0: The view window's left coord.
 * @y0: The view window's top coord.
 * @x1: The view window's right coord.
 * @y1: The view window's bottom coord.
 * @buf: The buffer where the bytemap is stored.
 * @rowstride: the rowstride for @buf.
 *
 * Each pixel gets a value proportional to the area within the pixel
 * overlapping the (filled) SVP. Pixel (x, y) is stored at:
 *
 *    @buf[(y - * @y0) * @rowstride + (x - @x0)]
 *
 * All pixels @x0 <= x < @x1, @y0 <= y < @y1 are generated. A
 * stored value of zero is no coverage, and a value of 255 is full
 * coverage. The area within the pixel (x, y) is the region covered
 * by [x..x+1] and [y..y+1].
 **/
void
art_gray_svp_aa (const ArtSVP *svp,
		 int x0, int y0, int x1, int y1,
		 art_u8 *buf, int rowstride)
{
  ArtGraySVPData data;

  data.buf = buf;
  data.rowstride = rowstride;
  data.x0 = x0;
  data.x1 = x1;
  art_svp_render_aa (svp, x0, y0, x1, y1, art_gray_svp_callback, &data);
}
