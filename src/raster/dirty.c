/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <string.h>

#include "raster/algo.h"
#include "raster/brush.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/mask.h"

#endif

#define ADD_COLUMN()							\
  row->cols++;								\
  row->col = jrealloc (row->col, sizeof (struct DirtyCol) * row->cols); \
									\
  if (u < row->cols-1)							\
    memmove (row->col+u+1, row->col+u,					\
	     sizeof (struct DirtyCol) * (row->cols-1-u));

#define ADD_ROW()							\
  dirty->rows++;							\
  dirty->row = jrealloc (dirty->row,					\
			 sizeof (struct DirtyRow) * dirty->rows);	\
									\
  if (v < dirty->rows-1)						\
    memmove (dirty->row+v+1, dirty->row+v,				\
	     sizeof (struct DirtyRow) * (dirty->rows-1-v));

#define RESTORE_IMAGE(u)			\
  if (row->col[u].flags & DIRTY_VALID_COLUMN) {	\
    row->col[u].flags ^= DIRTY_VALID_COLUMN;	\
						\
    memcpy (row->col[u].ptr,			\
	    row->col[u].data,			\
	    DIRTY_LINE_SIZE (row->col[u].w));	\
  }

#define JOIN_WITH_NEXT()						\
  {									\
    RESTORE_IMAGE (u+1);						\
									\
    row->col[u].w += row->col[u+1].w;					\
									\
    if (row->col[u+1].data)						\
      jfree (row->col[u+1].data);					\
									\
    if (u+1 < row->cols-1)						\
      memmove (row->col+u+1, row->col+u+2,				\
	       sizeof (struct DirtyCol) * (row->cols-2-u));		\
									\
    row->cols--;							\
    row->col = jrealloc (row->col, sizeof (struct DirtyCol) * row->cols); \
  }

#define EAT_COLUMNS()							\
  to_x2 = x2;								\
									\
  /* "eat" columns in range */						\
  for (u2=u+1; u2<row->cols; u2++) {					\
    /* done (next column too far) */					\
    if (x2 < row->col[u2].x-1)						\
      break;								\
  }									\
									\
  u2--;									\
									\
  /* "eat" columns */							\
  if (u2 > u) {								\
    for (u3=u+1; u3<=u2; u3++) {					\
      RESTORE_IMAGE (u3);						\
      if (row->col[u3].data)						\
	jfree (row->col[u3].data);					\
    }									\
									\
    to_x2 = MAX (to_x2, row->col[u2].x+row->col[u2].w-1);		\
									\
    if (u2 < row->cols-1)						\
      memmove (row->col+u+1, row->col+u2+1,				\
	       sizeof (struct DirtyCol) * (row->cols-u2-1));		\
									\
    row->cols -= u2 - u;						\
    row->col = jrealloc (row->col,					\
			 sizeof (struct DirtyCol) * row->cols);		\
  }									\
									\
  row->col[u].w = to_x2 - row->col[u].x + 1;				\
  row->col[u].data = jrealloc (row->col[u].data,			\
			       DIRTY_LINE_SIZE (row->col[u].w));

typedef struct AlgoData
{
  Dirty *dirty;
  Brush *brush;
  int thickness;
} AlgoData;

static void algo_putpixel (int x, int y, AlgoData *data);
/* static void algo_putthick (int x, int y, AlgoData *data); */
static void algo_putbrush (int x, int y, AlgoData *data);

static void *swap_hline (Image *image);
static void swap_hline1 (void *image, void *data, int x1, int x2);
static void swap_hline2 (void *image, void *data, int x1, int x2);
static void swap_hline4 (void *image, void *data, int x1, int x2);

Dirty *dirty_new (Image *image, int x1, int y1, int x2, int y2, int tiled)
{
  Dirty *dirty;

  dirty = (Dirty *)jnew (Dirty, 1);
  if (!dirty)
    return NULL;

  dirty->image = image;
  dirty->x1 = MID (0, x1, image->w-1);
  dirty->y1 = MID (0, y1, image->h-1);
  dirty->x2 = MID (x1, x2, image->w-1);
  dirty->y2 = MID (y1, y2, image->h-1);
  dirty->tiled = tiled;
  dirty->rows = 0;
  dirty->row = NULL;
  dirty->mask = NULL;

  return dirty;
}

Dirty *dirty_new_copy (Dirty *src)
{
  int u, v, size;
  Dirty *dst;

  dst = dirty_new (src->image, src->x1, src->y1, src->x2, src->y2, src->tiled);
  if (!dst)
    return NULL;

  dst->rows = src->rows;
  dst->row = jmalloc (sizeof (struct DirtyRow) * src->rows);

  for (v=0; v<dst->rows; v++) {
    dst->row[v].y = src->row[v].y;
    dst->row[v].cols = src->row[v].cols;
    dst->row[v].col = jmalloc (sizeof (struct DirtyCol) * dst->row[v].cols);

    for (u=0; u<dst->row[v].cols; u++) {
      dst->row[v].col[u].x = src->row[v].col[u].x;
      dst->row[v].col[u].w = src->row[v].col[u].w;
      dst->row[v].col[u].flags = src->row[v].col[u].flags;
      dst->row[v].col[u].ptr = src->row[v].col[u].ptr;

      size = dst->row[v].col[u].w << IMAGE_SHIFT (dst->image);

      dst->row[v].col[u].data = jmalloc (size);

      memcpy (dst->row[v].col[u].data, src->row[v].col[u].data, size);
    }
  }

  return dst;
}

void dirty_free (Dirty *dirty)
{
  int u, v;

  if (dirty->row) {
    for (v=0; v<dirty->rows; v++) {
      for (u=0; u<dirty->row[v].cols; u++)
	jfree (dirty->row[v].col[u].data);

      jfree (dirty->row[v].col);
    }

    jfree (dirty->row);
  }

  jfree (dirty);
}

void dirty_putpixel (Dirty *dirty, int x, int y)
{
  struct DirtyRow *row;
  int u, v;

  /* clip */
  if (dirty->tiled) {
    if (x < 0)
      x = dirty->image->w - (-(x+1) % dirty->image->w) - 1;
    else
      x = x % dirty->image->w;

    if (y < 0)
      y = dirty->image->h - (-(y+1) % dirty->image->h) - 1;
    else
      y = y % dirty->image->h;
  }
  else {
    if ((x < dirty->x1) || (x > dirty->x2) ||
        (y < dirty->y1) || (y > dirty->y2))
      return;
  }

  if (dirty->mask) {
    if ((x < dirty->mask->x) || (x >= dirty->mask->x+dirty->mask->w) ||
	(y < dirty->mask->y) || (y >= dirty->mask->y+dirty->mask->h))
      return;

    if ((dirty->mask->bitmap) &&
	!(dirty->mask->bitmap->method->getpixel (dirty->mask->bitmap,
						 x-dirty->mask->x,
						 y-dirty->mask->y)))
      return;
  }

  /* check if the row exists */
  row = NULL;

  for (v=0; v<dirty->rows; v++)
    if (dirty->row[v].y == y) {
      row = dirty->row+v;
      break;
    }
    else if (dirty->row[v].y > y)
      break;

  /* we must add a new row? */
  if (!row) {
    ADD_ROW ();

    dirty->row[v].y = y;
    dirty->row[v].cols = 0;
    dirty->row[v].col = NULL;

    row = dirty->row+v;
  }

  /* go column by column */
  for (u=0; u<row->cols; u++) {
    /* inside a existent column */
    if ((x >= row->col[u].x) && (x <= row->col[u].x+row->col[u].w-1))
      return;
    /* the pixel is to right of the column */
    else if (x == row->col[u].x+row->col[u].w) {
      RESTORE_IMAGE (u);

      row->col[u].w++;

      /* there is a left column? */
      if (u < row->cols-1 && x+1 == row->col[u+1].x)
	JOIN_WITH_NEXT ();

      row->col[u].data = jrealloc (row->col[u].data,
				   DIRTY_LINE_SIZE (row->col[u].w));
      return;
    }
    /* the pixel is to left of the column */
    else if (x+1 == row->col[u].x) {
      RESTORE_IMAGE (u);

      row->col[u].x--;
      row->col[u].w++;
      row->col[u].data = jrealloc (row->col[u].data,
				   DIRTY_LINE_SIZE (row->col[u].w));

      row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x, y);
      return;
    }
    /* the next column is more too far */
    else if (x < row->col[u].x)
      break;
  }

  /* add a new column */
  ADD_COLUMN ();

  row->col[u].x = x;
  row->col[u].w = 1;
  row->col[u].flags = 0;
  row->col[u].data = jmalloc (DIRTY_LINE_SIZE (1));
  row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x, y);
}

void dirty_hline (Dirty *dirty, int x1, int y, int x2)
{
  struct DirtyRow *row;
  int x, u, v, w/* , len */;
/*   int u1, u2; */
  int u2, u3, to_x2;

  if (dirty->tiled) {
    if (x1 > x2)
      return;

    dirty->tiled = FALSE;

    if (y < 0)
      y = dirty->image->h - (-(y+1) % dirty->image->h) - 1;
    else
      y = y % dirty->image->h;

    w = x2-x1+1;
    if (w >= dirty->image->w)
      dirty_hline (dirty, 0, y, dirty->image->w-1);
    else {
      x = x1;
      if (x < 0)
	x = dirty->image->w - (-(x+1) % dirty->image->w) - 1;
      else
	x = x % dirty->image->w;

      if (x+w-1 <= dirty->image->w-1)
	dirty_hline (dirty, x, y, x+w-1);
      else {
        dirty_hline (dirty, x, y, dirty->image->w-1);
        dirty_hline (dirty, 0, y, w-(dirty->image->w-x)-1);
      }
    }

    dirty->tiled = TRUE;
    return;
  }

  /* clip */
  if ((y < dirty->y1) || (y > dirty->y2))
    return;

  if (x1 < dirty->x1)
    x1 = dirty->x1;

  if (x2 > dirty->x2)
    x2 = dirty->x2;

  if (x1 > x2)
    return;

  /* mask clip */
  if (dirty->mask) {
    if ((y < dirty->mask->y) || (y >= dirty->mask->y+dirty->mask->h))
      return;

    if (x1 < dirty->mask->x)
      x1 = dirty->mask->x;

    if (x2 > dirty->mask->x+dirty->mask->w-1)
      x2 = dirty->mask->x+dirty->mask->w-1;

    if (dirty->mask->bitmap) {
      for (; x1<=x2; x1++)
	if (dirty->mask->bitmap->method->getpixel (dirty->mask->bitmap,
						   x1-dirty->mask->x,
						   y-dirty->mask->y))
	  break;

      for (; x2>=x1; x2--)
	if (dirty->mask->bitmap->method->getpixel (dirty->mask->bitmap,
						   x2-dirty->mask->x,
						   y-dirty->mask->y))
	  break;
    }

    if (x1 > x2)
      return;
  }

  /* check if the row exists */
  row = NULL;

  for (v=0; v<dirty->rows; v++)
    if (dirty->row[v].y == y) {
      row = dirty->row+v;
      break;
    }
    else if (dirty->row[v].y > y)
      break;

  /* we must add a new row? */
  if (!row) {
    ADD_ROW ();

    dirty->row[v].y = y;
    dirty->row[v].cols = 0;
    dirty->row[v].col = NULL;

    row = dirty->row+v;
  }

  /**********************************************************************/
  /* HLINE for Dirty with mask */
  
  if (dirty->mask && dirty->mask->bitmap) {
    for (x=x1; x<=x2; x++) {
      if (!dirty->mask->bitmap->method->getpixel (dirty->mask->bitmap,
						  x-dirty->mask->x,
						  y-dirty->mask->y)) {
	continue;
      }

      /* check if the pixel is inside some column */
      for (u=0; u<row->cols; u++) {
	/* inside a existent column */
	if ((x >= row->col[u].x) && (x <= row->col[u].x+row->col[u].w-1))
	  goto done;
	/* the pixel is in the right of the column */
	else if (x == row->col[u].x+row->col[u].w) {
	  RESTORE_IMAGE (u);

	  row->col[u].w++;

	  /* there is a left column? */
	  if (u < row->cols-1 && x == row->col[u+1].x-1) {
	    if (row->col[u+1].flags & DIRTY_VALID_COLUMN) {
	      row->col[u+1].flags ^= DIRTY_VALID_COLUMN;

	      memcpy (row->col[u+1].ptr,
		      row->col[u+1].data,
		      DIRTY_LINE_SIZE (row->col[u+1].w));
	    }

	    row->col[u].w += row->col[u+1].w;

	    /* remove the col[u+1] */
	    if (row->col[u+1].data)
	      jfree (row->col[u+1].data);

	    if (u+1 < row->cols-1)
	      memmove (row->col+u+1, row->col+u+2,
		       sizeof (struct DirtyCol) * (row->cols-2-u));

	    row->cols--;
	    row->col = jrealloc (row->col, sizeof (struct DirtyCol) * row->cols);
	  }

	  row->col[u].data = jrealloc (row->col[u].data,
				       DIRTY_LINE_SIZE (row->col[u].w));
	  goto done;
	}
	/* the pixel is in the left of the column */
	else if (x == row->col[u].x-1) {
	  if (row->col[u].flags & DIRTY_VALID_COLUMN) {
	    row->col[u].flags ^= DIRTY_VALID_COLUMN;

	    memcpy (row->col[u].ptr,
		    row->col[u].data,
		    DIRTY_LINE_SIZE (row->col[u].w));
	  }

	  row->col[u].x--;
	  row->col[u].w++;
	  row->col[u].data = jrealloc (row->col[u].data,
				       DIRTY_LINE_SIZE (row->col[u].w));

	  row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x, y);
	  goto done;
	}
	else if (x < row->col[u].x)
	  break;
      }

      /* add a new column */
      row->cols++;
      row->col = jrealloc (row->col, sizeof (struct DirtyCol) * row->cols);

      if (u < row->cols-1)
	memmove (row->col+u+1, row->col+u,
		 sizeof (struct DirtyCol) * (row->cols-1-u));

      row->col[u].x = x;
      row->col[u].w = 1;
      row->col[u].flags = 0;
      row->col[u].data = jmalloc (DIRTY_LINE_SIZE (1));
      row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x, y);

    done:;
    }
  }

  /**********************************************************************/
  /* HLINE for Dirty without mask */

  else {
    /* go column by column */
    for (u=0; u<row->cols; u++) {
      /* the hline is to right of the column */
      if ((x1 >= row->col[u].x) && (x1 <= row->col[u].x+row->col[u].w)) {
	/* inside the column */
	if (x2 <= row->col[u].x+row->col[u].w-1)
	  return;
	/* extend this column to "x2" */
	else {
	  RESTORE_IMAGE (u);
	  EAT_COLUMNS ();
	  return;
	}
      }
      /* the hline is to left of the column */
      else if ((x1 < row->col[u].x) && (x2 >= row->col[u].x-1)) {
	RESTORE_IMAGE (u);

	/* extend to left */
	row->col[u].w += row->col[u].x - x1;
	row->col[u].x = x1;
	row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x1, y);

	/* inside the column */
	if (x2 <= row->col[u].x+row->col[u].w-1) {
	  row->col[u].data = jrealloc (row->col[u].data,
				       DIRTY_LINE_SIZE (row->col[u].w));
	  return;
	}
	/* extend this column to "x2" */
	else {
	  EAT_COLUMNS ();
	  return;
	}
      }
      /* the next column is more too far */
      else if (x2 < row->col[u].x-1)
	break;
    }

    /* add a new column */
    ADD_COLUMN ();

    row->col[u].x = x1;
    row->col[u].w = x2-x1+1;
    row->col[u].flags = 0;
    row->col[u].data = jmalloc (DIRTY_LINE_SIZE (row->col[u].w));
    row->col[u].ptr = IMAGE_ADDRESS (dirty->image, x1, y);
  }
}

void dirty_vline (Dirty *dirty, int x, int y1, int y2)
{
  dirty_line (dirty, x, y1, x, y2);
}

void dirty_line (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, NULL, 0 };
  algo_line (x1, y1, x2, y2, &data, (AlgoPixel)algo_putpixel);
}

void dirty_rect (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  dirty_hline (dirty, x1, y1, x2);
  dirty_hline (dirty, x1, y2, x2);
  dirty_vline (dirty, x1, y1, y2);
  dirty_vline (dirty, x2, y1, y2);
}

void dirty_rectfill (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  int y;

  for (y=y1; y<=y2; y++)
    dirty_hline (dirty, x1, y, x2);
}

/* void dirty_putpixel_thick (Dirty *dirty, int x, int y, int thickness) */
/* { */
/*   AlgoData data = { dirty, NULL, thickness }; */
/*   algo_putthick (x, y, &data); */
/* } */

/* void dirty_line_thick (Dirty *dirty, int x1, int y1, int x2, int y2, int thickness) */
/* { */
/*   AlgoData data = { dirty, NULL, thickness }; */
/*   algo_line (x1, y1, x2, y2, &data, */
/* 	     (thickness == 1)? */
/* 	     (AlgoPixel)algo_putpixel: */
/* 	     (AlgoPixel)algo_putthick); */
/* } */

void dirty_putpixel_brush (Dirty *dirty, Brush *brush, int x, int y)
{
  AlgoData data = { dirty, brush, 0 };
  if (brush->size == 1)
    algo_putpixel (x, y, &data);
  else
    algo_putbrush (x, y, &data);
}

void dirty_hline_brush (Dirty *dirty, struct Brush *brush, int x1, int y, int x2)
{
  AlgoData data = { dirty, brush, 0 };
  int x;

  if (brush->size == 1)
    for (x=x1; x<=x2; x++)
      algo_putpixel (x, y, &data);
  else
    for (x=x1; x<=x2; x++)
      algo_putbrush (x, y, &data);
}

void dirty_line_brush (Dirty *dirty, Brush *brush, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, brush, 0 };
  algo_line (x1, y1, x2, y2, &data,
	     (brush->size == 1)?
	     (AlgoPixel)algo_putpixel:
	     (AlgoPixel)algo_putbrush);
}

/* static void remove_column (Dirty *dirty, int u, int v) */
/* { */
/*   jfree (dirty->row[v].col[u].data); */

/*   memmove (dirty->row[v].col+u, dirty->row[v].col+u+1, */
/* 	   sizeof (struct DirtyCol) * ((dirty->row[v].cols - u) - 1)); */

/*   dirty->row[v].cols--; */
/*   dirty->row[v].col = */
/*     jrealloc (dirty->row[v].col, */
/* 	       sizeof (struct DirtyCol) * dirty->row[v].cols); */
/* } */

/* joins all consecutive columns */
/* void dirty_optimize (Dirty *dirty) */
/* { */
/*   int u, v, w; */

/*   for (v=0; v<dirty->rows; v++) { */
/*     for (u=0; u<dirty->row[v].cols; u++) { */
/*       for (w=0; w<dirty->row[v].cols; w++) { */
/*         if (u == w) */
/*           continue; */

/*         if ((dirty->row[v].col[u].x+dirty->row[v].col[u].w == dirty->row[v].col[w].x)) { */
/* 	  int oldw = dirty->row[v].col[u].w; */

/*           dirty->row[v].col[u].w += dirty->row[v].col[w].w; */
/* 	  dirty->row[v].col[u].data = */
/* 	    jrealloc (dirty->row[v].col[u].data, */
/* 		       DIRTY_LINE_SIZE (dirty->row[v].col[u].w)); */

/* 	  memcpy (dirty->row[v].col[u].data + DIRTY_LINE_SIZE (oldw), */
/* 		  dirty->row[v].col[w].data, */
/* 		  DIRTY_LINE_SIZE (dirty->row[v].col[w].w)); */

/* 	  dirty->row[v].col[u].flags |= DIRTY_VALID_COLUMN; */
/* 	  dirty->row[v].col[u].flags |= DIRTY_MUSTBE_UPDATED; */

/*           remove_column (dirty, w, v); */
/*           u = -1; */
/*           break; */
/*         } */
/*       } */
/*     } */
/*   } */
/* } */

void dirty_get (Dirty *dirty)
{
  int u, v, shift;

  shift = IMAGE_SHIFT (dirty->image);

  for (v=0; v<dirty->rows; v++)
    for (u=0; u<dirty->row[v].cols; u++) {
      if (!(dirty->row[v].col[u].flags & DIRTY_VALID_COLUMN)) {
	memcpy (dirty->row[v].col[u].data,
		dirty->row[v].col[u].ptr,
		dirty->row[v].col[u].w<<shift);

	dirty->row[v].col[u].flags |= DIRTY_VALID_COLUMN;
	dirty->row[v].col[u].flags |= DIRTY_MUSTBE_UPDATED;
      }
    }
}

void dirty_put (Dirty *dirty)
{
  int u, v, shift;

  shift = IMAGE_SHIFT (dirty->image);

  for (v=0; v<dirty->rows; v++)
    for (u=0; u<dirty->row[v].cols; u++) {
      if (dirty->row[v].col[u].flags & DIRTY_VALID_COLUMN) {
	memcpy (dirty->row[v].col[u].ptr,
		dirty->row[v].col[u].data,
		dirty->row[v].col[u].w<<shift);
      }
    }
}

void dirty_swap (Dirty *dirty)
{
  void (*proc) (void *image, void *data, int x1, int x2);
  int u, v;

  proc = swap_hline (dirty->image);

  for (v=0; v<dirty->rows; v++)
    for (u=0; u<dirty->row[v].cols; u++) {
      if (dirty->row[v].col[u].flags & DIRTY_VALID_COLUMN) {
	(*proc) (dirty->row[v].col[u].ptr,
		 dirty->row[v].col[u].data,
		 dirty->row[v].col[u].x,
		 dirty->row[v].col[u].x+dirty->row[v].col[u].w-1);
      }
    }
}

static void algo_putpixel (int x, int y, AlgoData *data)
{
  dirty_putpixel (data->dirty, x, y);
}

/* static void algo_putthick (int x, int y, AlgoData *data) */
/* { */
/*   register int t1 = -data->thickness/2; */
/*   register int t2 = t1+data->thickness-1; */

/*   dirty_rectfill (data->dirty, x+t1, y+t1, x+t2, y+t2); */
/* } */

static void algo_putbrush (int x, int y, AlgoData *data)
{
  register struct BrushScanline *scanline = data->brush->scanline;
  register int c = data->brush->size/2;

  x -= c;
  y -= c;

  for (c=0; c<data->brush->size; c++) {
    if (scanline->state)
      dirty_hline (data->dirty, x+scanline->x1, y+c, x+scanline->x2);
    scanline++;
  }
}

static void *swap_hline (Image *image)
{
  void *proc;
  switch (image->imgtype) {
    case IMAGE_RGB:       proc = swap_hline4; break;
    case IMAGE_GRAYSCALE: proc = swap_hline2; break;
    case IMAGE_INDEXED:   proc = swap_hline1; break;
    default:
      proc = NULL;
  }
  return proc;
}

static void swap_hline1 (void *image, void *data, int x1, int x2)
{
  unsigned char *address1 = image;
  unsigned char *address2 = data;
  register int c;
  int x;

  for (x=x1; x<=x2; x++) {
    c = *address1;
    *address1 = *address2;
    *address2 = c;
    address1++;
    address2++;
  }
}

static void swap_hline2 (void *image, void *data, int x1, int x2)
{
  unsigned short *address1 = image;
  unsigned short *address2 = data;
  register int c;
  int x;

  for (x=x1; x<=x2; x++) {
    c = *address1;
    *address1 = *address2;
    *address2 = c;
    address1++;
    address2++;
  }
}

static void swap_hline4 (void *image, void *data, int x1, int x2)
{
  unsigned long *address1 = image;
  unsigned long *address2 = data;
  register int c;
  int x;

  for (x=x1; x<=x2; x++) {
    c = *address1;
    *address1 = *address2;
    *address2 = c;
    address1++;
    address2++;
  }
}
