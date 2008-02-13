/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <string.h>

#include "raster/algo.h"
#include "raster/brush.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/mask.h"

#define ADD_COLUMN(row, col, u)						\
  ++row->cols;								\
  row->col = jrealloc(row->col, sizeof(struct DirtyCol) * row->cols);	\
  col = row->col+u;							\
  if (u < row->cols-1)							\
    memmove(col+1,							\
	    col,							\
	    sizeof(struct DirtyCol) * (row->cols-1-u));

#define ADD_ROW(dirty, row, v)						\
  ++dirty->rows;							\
  dirty->row = jrealloc(dirty->row,					\
			sizeof(struct DirtyRow) * dirty->rows);		\
  row = dirty->row+v;							\
  if (v < dirty->rows-1)						\
    memmove(row+1,							\
	    row,							\
	    sizeof(struct DirtyRow) * (dirty->rows-1-v));

#define RESTORE_IMAGE(col)			\
  if ((col)->flags & DIRTY_VALID_COLUMN) {	\
    (col)->flags ^= DIRTY_VALID_COLUMN;		\
						\
    memcpy((col)->ptr,				\
	   (col)->data,				\
	   DIRTY_LINE_SIZE((col)->w));		\
  }

#define JOIN_WITH_NEXT(row, col, u)					\
  {									\
    RESTORE_IMAGE(col+1);						\
									\
    col->w += (col+1)->w;						\
									\
    if ((col+1)->data)							\
      jfree((col+1)->data);						\
									\
    if (u+1 < row->cols-1)						\
      memmove(row->col+u+1,						\
	      row->col+u+2,						\
	      sizeof(struct DirtyCol) * (row->cols-2-u));		\
									\
    row->cols--;							\
    row->col = jrealloc(row->col, sizeof(struct DirtyCol) * row->cols);	\
    col = row->col+u;							\
  }

#define EAT_COLUMNS()							\
  to_x2 = x2;								\
									\
  /* "eat" columns in range */						\
  for (u2=u+1; u2<row->cols; ++u2) {					\
    /* done (next column too far) */					\
    if (x2 < row->col[u2].x-1)						\
      break;								\
  }									\
									\
  u2--;									\
									\
  /* "eat" columns */							\
  if (u2 > u) {								\
    for (u3=u+1; u3<=u2; ++u3) {					\
      RESTORE_IMAGE(row->col+u3);					\
      if (row->col[u3].data)						\
	jfree(row->col[u3].data);					\
    }									\
									\
    to_x2 = MAX(to_x2, row->col[u2].x+row->col[u2].w-1);		\
									\
    if (u2 < row->cols-1)						\
      memmove(row->col+u+1,						\
	      row->col+u2+1,						\
	      sizeof(struct DirtyCol) * (row->cols-u2-1));		\
									\
    row->cols -= u2 - u;						\
    row->col = jrealloc(row->col,					\
			sizeof(struct DirtyCol) * row->cols);		\
    col = row->col+u;							\
  }									\
									\
  row->col[u].w = to_x2 - row->col[u].x + 1;				\
  row->col[u].data = jrealloc(row->col[u].data,				\
			      DIRTY_LINE_SIZE(row->col[u].w));

typedef struct AlgoData
{
  Dirty *dirty;
  Brush *brush;
  int thickness;
} AlgoData;

static void algo_putpixel(int x, int y, AlgoData *data);
/* static void algo_putthick(int x, int y, AlgoData *data); */
static void algo_putbrush(int x, int y, AlgoData *data);

static void *swap_hline(Image *image);
static void swap_hline32(void *image, void *data, int x1, int x2);
static void swap_hline16(void *image, void *data, int x1, int x2);
static void swap_hline8(void *image, void *data, int x1, int x2);

Dirty *dirty_new(Image *image, int x1, int y1, int x2, int y2, int tiled)
{
  Dirty *dirty;

  dirty = (Dirty *)jnew(Dirty, 1);
  if (!dirty)
    return NULL;

  dirty->image = image;
  dirty->x1 = MID(0, x1, image->w-1);
  dirty->y1 = MID(0, y1, image->h-1);
  dirty->x2 = MID(x1, x2, image->w-1);
  dirty->y2 = MID(y1, y2, image->h-1);
  dirty->tiled = tiled;
  dirty->rows = 0;
  dirty->row = NULL;
  dirty->mask = NULL;

  return dirty;
}

Dirty *dirty_new_copy(Dirty *src)
{
  int u, v, size;
  Dirty *dst;

  dst = dirty_new(src->image, src->x1, src->y1, src->x2, src->y2, src->tiled);
  if (!dst)
    return NULL;

  dst->rows = src->rows;
  dst->row = jmalloc(sizeof (struct DirtyRow) * src->rows);

  for (v=0; v<dst->rows; ++v) {
    dst->row[v].y = src->row[v].y;
    dst->row[v].cols = src->row[v].cols;
    dst->row[v].col = jmalloc(sizeof(struct DirtyCol) * dst->row[v].cols);

    for (u=0; u<dst->row[v].cols; ++u) {
      dst->row[v].col[u].x = src->row[v].col[u].x;
      dst->row[v].col[u].w = src->row[v].col[u].w;
      dst->row[v].col[u].flags = src->row[v].col[u].flags;
      dst->row[v].col[u].ptr = src->row[v].col[u].ptr;

      size = dst->row[v].col[u].w << IMAGE_SHIFT(dst->image);

      dst->row[v].col[u].data = jmalloc(size);

      memcpy(dst->row[v].col[u].data, src->row[v].col[u].data, size);
    }
  }

  return dst;
}

void dirty_free(Dirty *dirty)
{
  register struct DirtyRow *row;
  register struct DirtyCol *col;
  struct DirtyRow *rowend;
  struct DirtyCol *colend;

  if (dirty->row) {
    row = dirty->row;
    rowend = row+dirty->rows;
    for (; row<rowend; ++row) {
      col = row->col;
      colend = col+row->cols;
      for (; col<colend; ++col)
	jfree(col->data);

      jfree(row->col);
    }

    jfree(dirty->row);
  }
  jfree(dirty);
}

void dirty_putpixel(Dirty *dirty, int x, int y)
{
  struct DirtyRow *row;		/* row=dirty->row+v */
  struct DirtyCol *col;		/* col=row->col+u */
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
	!(dirty->mask->bitmap->method->getpixel(dirty->mask->bitmap,
						x-dirty->mask->x,
						y-dirty->mask->y)))
      return;
  }

  /* check if the row exists */
  row = NULL;

  for (v=0; v<dirty->rows; ++v)
    if (dirty->row[v].y == y) {
      row = dirty->row+v;
      break;
    }
    else if (dirty->row[v].y > y)
      break;

  /* we must add a new row? */
  if (!row) {
    ADD_ROW(dirty, row, v);

    row->y = y;
    row->cols = 0;
    row->col = NULL;
  }

  /* go column by column */
  col = row->col;
  for (u=0; u<row->cols; ++u, ++col) {
    /* inside a existent column */
    if ((x >= col->x) && (x <= col->x+col->w-1))
      return;
    /* the pixel is to right of the column */
    else if (x == col->x+col->w) {
      RESTORE_IMAGE(col);

      ++col->w;

      /* there is a left column? */
      if (u < row->cols-1 && x+1 == (col+1)->x)
	JOIN_WITH_NEXT(row, col, u);

      col->data = jrealloc(col->data, DIRTY_LINE_SIZE(col->w));
      return;
    }
    /* the pixel is to left of the column */
    else if (x+1 == col->x) {
      RESTORE_IMAGE(col);

      --col->x;
      ++col->w;
      col->data = jrealloc(col->data, DIRTY_LINE_SIZE(col->w));

      col->ptr = IMAGE_ADDRESS(dirty->image, x, y);
      return;
    }
    /* the next column is more too far */
    else if (x < col->x)
      break;
  }

  /* add a new column */
  ADD_COLUMN(row, col, u);

  col->x = x;
  col->w = 1;
  col->flags = 0;
  col->data = jmalloc(DIRTY_LINE_SIZE(1));
  col->ptr = IMAGE_ADDRESS(dirty->image, x, y);
}

void dirty_hline(Dirty *dirty, int x1, int y, int x2)
{
  struct DirtyRow *row;		/* row=dirty->row+v */
  struct DirtyCol *col;		/* col=row->col+u */
  int x, u, v, w;
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
      dirty_hline(dirty, 0, y, dirty->image->w-1);
    else {
      x = x1;
      if (x < 0)
	x = dirty->image->w - (-(x+1) % dirty->image->w) - 1;
      else
	x = x % dirty->image->w;

      if (x+w-1 <= dirty->image->w-1)
	dirty_hline(dirty, x, y, x+w-1);
      else {
        dirty_hline(dirty, x, y, dirty->image->w-1);
        dirty_hline(dirty, 0, y, w-(dirty->image->w-x)-1);
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
      for (; x1<=x2; ++x1)
	if (dirty->mask->bitmap->method->getpixel(dirty->mask->bitmap,
						  x1-dirty->mask->x,
						  y-dirty->mask->y))
	  break;

      for (; x2>=x1; x2--)
	if (dirty->mask->bitmap->method->getpixel(dirty->mask->bitmap,
						  x2-dirty->mask->x,
						  y-dirty->mask->y))
	  break;
    }

    if (x1 > x2)
      return;
  }

  /* check if the row exists */
  row = NULL;

  for (v=0; v<dirty->rows; ++v)
    if (dirty->row[v].y == y) {
      row = dirty->row+v;
      break;
    }
    else if (dirty->row[v].y > y)
      break;

  /* we must add a new row? */
  if (!row) {
    ADD_ROW(dirty, row, v);

    row->y = y;
    row->cols = 0;
    row->col = NULL;
  }

  /**********************************************************************/
  /* HLINE for Dirty with mask */
  
  if (dirty->mask && dirty->mask->bitmap) {
    for (x=x1; x<=x2; ++x) {
      if (!dirty->mask->bitmap->method->getpixel(dirty->mask->bitmap,
						 x-dirty->mask->x,
						 y-dirty->mask->y)) {
	continue;
      }

      /* check if the pixel is inside some column */
      col = row->col;
      for (u=0; u<row->cols; ++u, ++col) {
	/* inside a existent column */
	if ((x >= col->x) && (x <= col->x+col->w-1))
	  goto done;
	/* the pixel is in the right of the column */
	else if (x == col->x+col->w) {
	  RESTORE_IMAGE(col);

	  ++col->w;

	  /* there is a left column? */
	  if (u < row->cols-1 && x == (col+1)->x-1) {
	    if ((col+1)->flags & DIRTY_VALID_COLUMN) {
	      (col+1)->flags ^= DIRTY_VALID_COLUMN;

	      memcpy((col+1)->ptr,
		     (col+1)->data,
		     DIRTY_LINE_SIZE((col+1)->w));
	    }

	    col->w += (col+1)->w;

	    /* remove the col[u+1] */
	    if ((col+1)->data)
	      jfree((col+1)->data);

	    if (u+1 < row->cols-1)
	      memmove(col+1, col+2,
		      sizeof(struct DirtyCol) * (row->cols-2-u));

	    row->cols--;
	    row->col = jrealloc(row->col, sizeof(struct DirtyCol) * row->cols);
	    col = row->col+u;
	  }

	  col->data = jrealloc(col->data, DIRTY_LINE_SIZE(col->w));
	  goto done;
	}
	/* the pixel is in the left of the column */
	else if (x == col->x-1) {
	  if (col->flags & DIRTY_VALID_COLUMN) {
	    col->flags ^= DIRTY_VALID_COLUMN;

	    memcpy(col->ptr,
		   col->data,
		   DIRTY_LINE_SIZE(col->w));
	  }

	  --col->x;
	  ++col->w;
	  col->data = jrealloc(col->data, DIRTY_LINE_SIZE(col->w));
	  col->ptr = IMAGE_ADDRESS(dirty->image, x, y);
	  goto done;
	}
	else if (x < col->x)
	  break;
      }

      /* add a new column */
      ++row->cols;
      row->col = jrealloc(row->col, sizeof(struct DirtyCol) * row->cols);
      col = row->col+u;

      if (u < row->cols-1)
	memmove(col+1, col, sizeof(struct DirtyCol) * (row->cols-1-u));

      col->x = x;
      col->w = 1;
      col->flags = 0;
      col->data = jmalloc(DIRTY_LINE_SIZE(1));
      col->ptr = IMAGE_ADDRESS(dirty->image, x, y);

    done:;
    }
  }

  /**********************************************************************/
  /* HLINE for Dirty without mask */

  else {
    /* go column by column */
    col = row->col;
    for (u=0; u<row->cols; ++u, ++col) {
      /* the hline is to right of the column */
      if ((x1 >= col->x) && (x1 <= col->x+col->w)) {
	/* inside the column */
	if (x2 <= col->x+col->w-1)
	  return;
	/* extend this column to "x2" */
	else {
	  RESTORE_IMAGE(col);
	  EAT_COLUMNS();
	  return;
	}
      }
      /* the hline is to left of the column */
      else if ((x1 < col->x) && (x2 >= col->x-1)) {
	RESTORE_IMAGE(col);

	/* extend to left */
	col->w += col->x - x1;
	col->x = x1;
	col->ptr = IMAGE_ADDRESS(dirty->image, x1, y);

	/* inside the column */
	if (x2 <= col->x+col->w-1) {
	  col->data = jrealloc(col->data, DIRTY_LINE_SIZE(col->w));
	  return;
	}
	/* extend this column to "x2" */
	else {
	  EAT_COLUMNS();
	  return;
	}
      }
      /* the next column is more too far */
      else if (x2 < col->x-1)
	break;
    }

    /* add a new column */
    ADD_COLUMN(row, col, u);

    col->x = x1;
    col->w = x2-x1+1;
    col->flags = 0;
    col->data = jmalloc(DIRTY_LINE_SIZE(col->w));
    col->ptr = IMAGE_ADDRESS(dirty->image, x1, y);
  }
}

void dirty_vline(Dirty *dirty, int x, int y1, int y2)
{
  dirty_line(dirty, x, y1, x, y2);
}

void dirty_line(Dirty *dirty, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, NULL, 0 };
  algo_line(x1, y1, x2, y2, &data, (AlgoPixel)algo_putpixel);
}

void dirty_rect(Dirty *dirty, int x1, int y1, int x2, int y2)
{
  dirty_hline(dirty, x1, y1, x2);
  dirty_hline(dirty, x1, y2, x2);
  dirty_vline(dirty, x1, y1, y2);
  dirty_vline(dirty, x2, y1, y2);
}

void dirty_rectfill(Dirty *dirty, int x1, int y1, int x2, int y2)
{
  int y;

  for (y=y1; y<=y2; ++y)
    dirty_hline(dirty, x1, y, x2);
}

void dirty_putpixel_brush(Dirty *dirty, Brush *brush, int x, int y)
{
  AlgoData data = { dirty, brush, 0 };
  if (brush->size == 1)
    algo_putpixel(x, y, &data);
  else
    algo_putbrush(x, y, &data);
}

void dirty_hline_brush(Dirty *dirty, struct Brush *brush, int x1, int y, int x2)
{
  AlgoData data = { dirty, brush, 0 };
  int x;

  if (brush->size == 1)
    for (x=x1; x<=x2; ++x)
      algo_putpixel(x, y, &data);
  else
    for (x=x1; x<=x2; ++x)
      algo_putbrush(x, y, &data);
}

void dirty_line_brush(Dirty *dirty, Brush *brush, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, brush, 0 };
  algo_line(x1, y1, x2, y2, &data,
	    (brush->size == 1)?
	    (AlgoPixel)algo_putpixel:
	    (AlgoPixel)algo_putbrush);
}

void dirty_get(Dirty *dirty)
{
  register int v, u, shift = IMAGE_SHIFT(dirty->image);

  for (v=0; v<dirty->rows; ++v)
    for (u=0; u<dirty->row[v].cols; ++u) {
      if (!(dirty->row[v].col[u].flags & DIRTY_VALID_COLUMN)) {
	memcpy(dirty->row[v].col[u].data,
	       dirty->row[v].col[u].ptr,
	       dirty->row[v].col[u].w<<shift);

	dirty->row[v].col[u].flags |= DIRTY_VALID_COLUMN;
	dirty->row[v].col[u].flags |= DIRTY_MUSTBE_UPDATED;
      }
    }
}

void dirty_put(Dirty *dirty)
{
  register struct DirtyRow *row;
  register struct DirtyCol *col;
  struct DirtyRow *rowend;
  struct DirtyCol *colend;
  int shift = IMAGE_SHIFT(dirty->image);

  row = dirty->row;
  rowend = row+dirty->rows;
  for (; row<rowend; ++row) {
    col = row->col;
    colend = col+row->cols;
    for (; col<colend; ++col) {
      if (col->flags & DIRTY_VALID_COLUMN) {
	memcpy(col->ptr,
	       col->data,
	       col->w<<shift);
      }
    }
  }
}

void dirty_swap(Dirty *dirty)
{
  register struct DirtyRow *row;
  register struct DirtyCol *col;
  struct DirtyRow *rowend;
  struct DirtyCol *colend;
  void (*proc)(void *image, void *data, int x1, int x2);

  proc = swap_hline(dirty->image);

  row = dirty->row;
  rowend = row+dirty->rows;
  for (; row<rowend; ++row) {
    col = row->col;
    colend = col+row->cols;
    for (; col<colend; ++col) {
      if (col->flags & DIRTY_VALID_COLUMN) {
	(*proc)(col->ptr,
		col->data,
		col->x,
		col->x+col->w-1);
      }
    }
  }
}


static void algo_putpixel(int x, int y, AlgoData *data)
{
  dirty_putpixel(data->dirty, x, y);
}

static void algo_putbrush(int x, int y, AlgoData *data)
{
  register struct BrushScanline *scanline = data->brush->scanline;
  register int c = data->brush->size/2;

  x -= c;
  y -= c;

  for (c=0; c<data->brush->size; ++c) {
    if (scanline->state)
      dirty_hline(data->dirty, x+scanline->x1, y+c, x+scanline->x2);
    ++scanline;
  }
}

static void *swap_hline(Image *image)
{
  register void *proc;
  switch (image->imgtype) {
    case IMAGE_RGB:       proc = swap_hline32; break;
    case IMAGE_GRAYSCALE: proc = swap_hline16; break;
    case IMAGE_INDEXED:   proc = swap_hline8; break;
    default:
      proc = NULL;
  }
  return proc;
}

#define SWAP_HLINE(type)			\
  register type *address1 = image;		\
  register type *address2 = data;		\
  register int c;				\
  int x;					\
						\
  for (x=x1; x<=x2; ++x) {			\
    c = *address1;				\
    *address1 = *address2;			\
    *address2 = c;				\
    ++address1;					\
    ++address2;					\
  }

static void swap_hline32(void *image, void *data, int x1, int x2)
{
  SWAP_HLINE(ase_uint32);
}

static void swap_hline16(void *image, void *data, int x1, int x2)
{
  SWAP_HLINE(ase_uint16);
}

static void swap_hline8(void *image, void *data, int x1, int x2)
{
  SWAP_HLINE(ase_uint8);
}
