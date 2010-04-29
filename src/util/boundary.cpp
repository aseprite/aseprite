/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Adapted to ASE by David Capello (2003-2010)
 * See "LEGAL.txt" for more information.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

/**********************************************************************/
/* for ASE */
#include <limits.h>
#include "jinete/jbase.h"
#include "raster/image.h"
#include "util/boundary.h"

#define g_renew		jrenew
#define g_new		jnew
#define gint		int
#define gboolean	bool
#define G_MAXINT	INT_MAX
#define g_message(msg)	{}

typedef const Image		PixelRegion;
/**********************************************************************/

/* half intensity for mask */
/* #define HALF_WAY 127 */

/* BoundSeg array growth parameter */
#define MAX_SEGS_INC  2048


/*  The array of vertical segments  */
static gint      *vert_segs      = NULL;

/*  The array of segments  */
static BoundSeg  *tmp_segs       = NULL;
static gint       num_segs       = 0;
static gint       max_segs       = 0;

/* static empty segment arrays */
static gint      *empty_segs_n   = NULL;
static gint      *empty_segs_c   = NULL;
static gint      *empty_segs_l   = NULL;
static gint       max_empty_segs = 0;


/*  local function prototypes  */
static void find_empty_segs     (PixelRegion  *maskPR,
				 gint          scanline,
				 gint          empty_segs[],
				 gint          max_empty,
				 gint         *num_empty,
				 BoundaryType  type,
				 gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2);
static void make_seg            (gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
				 gboolean      open);
static void allocate_vert_segs  (PixelRegion  *PR);
static void allocate_empty_segs (PixelRegion  *PR);
static void process_horiz_seg   (gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2,
				 gboolean      open);
static void make_horiz_segs     (gint          start,
				 gint          end,
				 gint          scanline,
				 gint          empty[],
				 gint          num_empty,
				 gboolean      top);
static void generate_boundary   (PixelRegion  *PR,
				 BoundaryType  type,
				 gint          x1,
				 gint          y1,
				 gint          x2,
				 gint          y2);


/*  Function definitions  */

static void
find_empty_segs (PixelRegion  *maskPR,
		 gint          scanline,
		 gint          empty_segs[],
		 gint          max_empty,
		 gint         *num_empty,
		 BoundaryType  type,
		 gint          x1,
		 gint          y1,
		 gint          x2,
		 gint          y2)
{
  ase_uint8 *data;
  int x;
  int start, end;
  int val, last;
  int endx, l_num_empty;
  div_t d;

  data  = NULL;
  start = 0;
  end   = 0;

  *num_empty = 0;

  if (scanline < 0 || scanline >= maskPR->h)
    {
      empty_segs[(*num_empty)++] = 0;
      empty_segs[(*num_empty)++] = G_MAXINT;
      return;
    }

  if (type == WithinBounds)
    {
      if (scanline < y1 || scanline >= y2)
	{
	  empty_segs[(*num_empty)++] = 0;
	  empty_segs[(*num_empty)++] = G_MAXINT;
	  return;
	}

      start = x1;
      end = x2;
    }
  else if (type == IgnoreBounds)
    {
      start = 0;
      end = maskPR->w;
      if (scanline < y1 || scanline >= y2)
	x2 = -1;
    }

  /*   tilex = -1; */
  empty_segs[(*num_empty)++] = 0;
  last = -1;

  l_num_empty = *num_empty;

  d = div (start, 8);
  data = ((ase_uint8 *)maskPR->line[scanline])+d.quot;

  for (x = start; x < end;)
    {
      endx = end;
      if (type == IgnoreBounds && (endx > x1 || x < x2))
	{
	  for (; x < endx; x++)
	    {
	      if (*data & (1<<d.rem))
		if (x >= x1 && x < x2)
		  val = -1;
		else
		  val = 1;
	      else
		val = -1;
	      
	      _image_bitmap_next_bit(d, data);

	      if (last != val)
		empty_segs[l_num_empty++] = x;
	      
	      last = val;
	    }
	}
      else
	{
	  for (; x < endx; x++)
	    {
	      if (*data & (1<<d.rem))
		val = 1;
	      else
		val = -1;
	      
	      _image_bitmap_next_bit(d, data);

	      if (last != val)
		empty_segs[l_num_empty++] = x;

	      last = val;
	    }
	}
    }
  *num_empty = l_num_empty;

  if (last > 0)
    empty_segs[(*num_empty)++] = x;

  empty_segs[(*num_empty)++] = G_MAXINT;
}


static void
make_seg (gint     x1,
	  gint     y1,
	  gint     x2,
	  gint     y2,
	  gboolean open)
{
  if (num_segs >= max_segs)
    {
      max_segs += MAX_SEGS_INC;

      tmp_segs = g_renew (BoundSeg, tmp_segs, max_segs);
    }

  tmp_segs[num_segs].x1 = x1;
  tmp_segs[num_segs].y1 = y1;
  tmp_segs[num_segs].x2 = x2;
  tmp_segs[num_segs].y2 = y2;
  tmp_segs[num_segs].open = open;
  num_segs ++;
}


static void
allocate_vert_segs (PixelRegion *PR)
{
  gint i;

  /*  allocate and initialize the vert_segs array  */
  vert_segs = g_renew (gint, vert_segs, PR->w + 1);

  for (i = 0; i <= PR->w; i++)
    vert_segs[i] = -1;
}


static void
allocate_empty_segs (PixelRegion *PR)
{
  gint need_num_segs;

  /*  find the maximum possible number of empty segments given the current mask  */
  need_num_segs = PR->w + 3;

  if (need_num_segs > max_empty_segs)
    {
      max_empty_segs = need_num_segs;

      empty_segs_n = g_renew (gint, empty_segs_n, max_empty_segs);
      empty_segs_c = g_renew (gint, empty_segs_c, max_empty_segs);
      empty_segs_l = g_renew (gint, empty_segs_l, max_empty_segs);
    }
}


static void
process_horiz_seg (gint     x1,
		   gint     y1,
		   gint     x2,
		   gint     y2,
		   gboolean open)
{
  /*  This procedure accounts for any vertical segments that must be
      drawn to close in the horizontal segments.                     */

  if (vert_segs[x1] >= 0)
    {
      make_seg (x1, vert_segs[x1], x1, y1, !open);
      vert_segs[x1] = -1;
    }
  else
    vert_segs[x1] = y1;

  if (vert_segs[x2] >= 0)
    {
      make_seg (x2, vert_segs[x2], x2, y2, open);
      vert_segs[x2] = -1;
    }
  else
    vert_segs[x2] = y2;

  make_seg (x1, y1, x2, y2, open);
}


static void
make_horiz_segs (gint start,
		 gint end,
		 gint scanline,
		 gint empty[],
		 gint num_empty,
		 gboolean top)
{
  gint empty_index;
  gint e_s, e_e;    /* empty segment start and end values */

  for (empty_index = 0; empty_index < num_empty; empty_index += 2)
    {
      e_s = *empty++;
      e_e = *empty++;
      if (e_s <= start && e_e >= end)
	process_horiz_seg (start, scanline, end, scanline, top);
      else if ((e_s > start && e_s < end) ||
	       (e_e < end && e_e > start))
	process_horiz_seg (MAX (e_s, start), scanline,
			   MIN (e_e, end), scanline, top);
    }
}


static void
generate_boundary (PixelRegion  *PR,
		   BoundaryType  type,
		   gint          x1,
		   gint          y1,
		   gint          x2,
		   gint          y2)
{
  gint  scanline;
  gint  i;
  gint  start, end;
  gint *tmp_segs;

  gint  num_empty_n = 0;
  gint  num_empty_c = 0;
  gint  num_empty_l = 0;

  start = 0;
  end   = 0;

  /*  array for determining the vertical line segments which must be drawn  */
  allocate_vert_segs (PR);

  /*  make sure there is enough space for the empty segment array  */
  allocate_empty_segs (PR);

  num_segs = 0;

  if (type == WithinBounds)
    {
      start = y1;
      end = y2;
    }
  else if (type == IgnoreBounds)
    {
      start = 0;
      end   = PR->h;
    }

  /*  Find the empty segments for the previous and current scanlines  */
  find_empty_segs (PR, start - 1, empty_segs_l,
		   max_empty_segs, &num_empty_l,
		   type, x1, y1, x2, y2);
  find_empty_segs (PR, start, empty_segs_c,
		   max_empty_segs, &num_empty_c,
		   type, x1, y1, x2, y2);

  for (scanline = start; scanline < end; scanline++)
    {
      /*  find the empty segment list for the next scanline  */
      find_empty_segs (PR, scanline + 1, empty_segs_n,
		       max_empty_segs, &num_empty_n,
		       type, x1, y1, x2, y2);

      /*  process the segments on the current scanline  */
      for (i = 1; i < num_empty_c - 1; i += 2)
	{
	  make_horiz_segs (empty_segs_c [i], empty_segs_c [i+1],
			   scanline, empty_segs_l, num_empty_l, true);
	  make_horiz_segs (empty_segs_c [i], empty_segs_c [i+1],
			   scanline+1, empty_segs_n, num_empty_n, false);
	}

      /*  get the next scanline of empty segments, swap others  */
      tmp_segs = empty_segs_l;
      empty_segs_l = empty_segs_c;
      num_empty_l = num_empty_c;
      empty_segs_c = empty_segs_n;
      num_empty_c = num_empty_n;
      empty_segs_n = tmp_segs;
    }
}


BoundSeg *
find_mask_boundary (PixelRegion  *maskPR,
		    int          *num_elems,
		    BoundaryType  type,
		    int           x1,
		    int           y1,
		    int           x2,
		    int           y2)
{
  BoundSeg *new_segs = NULL;

  /*  The mask paramater can be any PixelRegion.  If the region
   *  has more than 1 bytes/pixel, the last byte of each pixel is
   *  used to determine the boundary outline.
   */

  /*  Calculate the boundary  */
  generate_boundary (maskPR, type, x1, y1, x2, y2);

  /*  Set the number of X segments  */
  *num_elems = num_segs;

  /*  Make a copy of the boundary  */
  if (num_segs)
    {
      new_segs = g_new (BoundSeg, num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}


/************************/
/*  Sorting a Boundary  */

static int find_segment (BoundSeg *, int, int, int);

static int
find_segment (BoundSeg *segs,
	      gint      ns,
	      gint      x,
	      gint      y)
{
  gint index;

  for (index = 0; index < ns; index++)
    if (((segs[index].x1 == x && segs[index].y1 == y) || (segs[index].x2 == x && segs[index].y2 == y)) &&
	segs[index].visited == false)
      return index;

  return -1;
}


BoundSeg *
sort_boundary (BoundSeg *segs,
	       gint      ns,
	       gint     *num_groups)
{
  gint      i;
  gint      index;
  gint      x, y;
  gint      startx, starty;
  gint      empty = (num_segs == 0);
  BoundSeg *new_segs;

  index = 0;
  new_segs = NULL;

  for (i = 0; i < ns; i++)
    segs[i].visited = false;

  num_segs = 0;
  *num_groups = 0;
  while (! empty)
    {
      empty = true;

      /*  find the index of a non-visited segment to start a group  */
      for (i = 0; i < ns; i++)
	if (segs[i].visited == false)
	  {
	    index = i;
	    empty = false;
	    i = ns;
	  }

      if (! empty)
	{
	  make_seg (segs[index].x1, segs[index].y1,
		    segs[index].x2, segs[index].y2,
		    segs[index].open);
	  segs[index].visited = true;

	  startx = segs[index].x1;
	  starty = segs[index].y1;
	  x = segs[index].x2;
	  y = segs[index].y2;

	  while ((index = find_segment (segs, ns, x, y)) != -1)
	    {
	      /*  make sure ordering is correct  */
	      if (x == segs[index].x1 && y == segs[index].y1)
		{
		  make_seg (segs[index].x1, segs[index].y1,
			    segs[index].x2, segs[index].y2,
			    segs[index].open);
		  x = segs[index].x2;
		  y = segs[index].y2;
		}
	      else
		{
		  make_seg (segs[index].x2, segs[index].y2,
			    segs[index].x1, segs[index].y1,
			    segs[index].open);
		  x = segs[index].x1;
		  y = segs[index].y1;
		}

	      segs[index].visited = true;
	    }

	  if (x != startx || y != starty)
	    g_message ("sort_boundary(): Unconnected boundary group!");

	  /*  Mark the end of a group  */
	  *num_groups = *num_groups + 1;
	  make_seg (-1, -1, -1, -1, 0);
	}
    }

  /*  Make a copy of the boundary  */
  if (num_segs)
    {
      new_segs = g_new (BoundSeg, num_segs);
      memcpy (new_segs, tmp_segs, (sizeof (BoundSeg) * num_segs));
    }

  /*  Return the new boundary  */
  return new_segs;
}

/* dacap: for ASE to avoid informed memory leaks */
void boundary_exit()
{
  if (vert_segs != NULL) {
    jfree(vert_segs);
    vert_segs = NULL;
  }
  if (tmp_segs != NULL) {
    jfree(tmp_segs);
    tmp_segs = NULL;
  }
  if (empty_segs_n != NULL) {
    jfree(empty_segs_n);
    empty_segs_n = NULL;
  }
  if (empty_segs_c != NULL) {
    jfree(empty_segs_c);
    empty_segs_c = NULL;
  }
  if (empty_segs_l != NULL) {
    jfree(empty_segs_l);
    empty_segs_l = NULL;
  }
}
