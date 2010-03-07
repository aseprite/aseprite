/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
#include "raster/pen.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/mask.h"

#define ADD_COLUMN(row, col, u)						\
  ++row->cols;								\
  row->col = (DirtyCol*)jrealloc(row->col,				\
				 sizeof(DirtyCol) * row->cols);		\
  col = row->col+u;							\
  if (u < row->cols-1)							\
    memmove(col+1,							\
	    col,							\
	    sizeof(DirtyCol) * (row->cols-1-u));

#define ADD_ROW(dirty, row, v)						\
  ++dirty->rows;							\
  dirty->row = (DirtyRow*)jrealloc(dirty->row,				\
				   sizeof(DirtyRow) * dirty->rows);	\
  row = dirty->row+v;							\
  if (v < dirty->rows-1)						\
    memmove(row+1,							\
	    row,							\
	    sizeof(DirtyRow) * (dirty->rows-1-v));

#define RESTORE_IMAGE(col)			\
  if ((col)->flags & DIRTY_VALID_COLUMN) {	\
    (col)->flags ^= DIRTY_VALID_COLUMN;		\
						\
    memcpy((col)->ptr,				\
	   (col)->data,				\
	   dirty_line_size(dirty, (col)->w));	\
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
	      sizeof(DirtyCol) * (row->cols-2-u));			\
									\
    row->cols--;							\
    row->col = (DirtyCol*)jrealloc(row->col,				\
				   sizeof(DirtyCol) * row->cols);	\
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
	      sizeof(DirtyCol) * (row->cols-u2-1));			\
									\
    row->cols -= u2 - u;						\
    row->col = (DirtyCol*)jrealloc(row->col,				\
				   sizeof(DirtyCol) * row->cols);	\
    col = row->col+u;							\
  }									\
									\
  row->col[u].w = to_x2 - row->col[u].x + 1;				\
  row->col[u].data = jrealloc(row->col[u].data,				\
			      dirty_line_size(dirty, row->col[u].w));

typedef struct AlgoData
{
  Dirty* dirty;
  Pen* pen;
  int thickness;
} AlgoData;

typedef void (*HLineSwapper)(void*,void*,int,int);

static void algo_putpixel(int x, int y, AlgoData *data);
/* static void algo_putthick(int x, int y, AlgoData *data); */
static void algo_putpen(int x, int y, AlgoData *data);

static HLineSwapper swap_hline(Image* image);
static void swap_hline32(void* image, void* data, int x1, int x2);
static void swap_hline16(void* image, void* data, int x1, int x2);
static void swap_hline8(void* image, void* data, int x1, int x2);

Dirty* dirty_new(Image* image, int x1, int y1, int x2, int y2, bool tiled)
{
  Dirty* dirty;

  dirty = (Dirty*)jnew(Dirty, 1);
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

Dirty* dirty_new_copy(Dirty* src)
{
  int u, v, size;
  Dirty* dst;

  dst = dirty_new(src->image, src->x1, src->y1, src->x2, src->y2, src->tiled);
  if (!dst)
    return NULL;

  dst->rows = src->rows;
  dst->row = (DirtyRow*)jmalloc(sizeof(DirtyRow) * src->rows);

  for (v=0; v<dst->rows; ++v) {
    dst->row[v].y = src->row[v].y;
    dst->row[v].cols = src->row[v].cols;
    dst->row[v].col = (DirtyCol*)jmalloc(sizeof(DirtyCol) * dst->row[v].cols);

    for (u=0; u<dst->row[v].cols; ++u) {
      dst->row[v].col[u].x = src->row[v].col[u].x;
      dst->row[v].col[u].w = src->row[v].col[u].w;
      dst->row[v].col[u].flags = src->row[v].col[u].flags;
      dst->row[v].col[u].ptr = src->row[v].col[u].ptr;

      size = dst->row[v].col[u].w << image_shift(dst->image);

      dst->row[v].col[u].data = jmalloc(size);

      memcpy(dst->row[v].col[u].data, src->row[v].col[u].data, size);
    }
  }

  return dst;
}

Dirty* dirty_new_from_differences(Image* image, Image* image_diff)
{
  Dirty* dirty = dirty_new(image, 0, 0, image->w, image->h, false);
  int x, y, x1, x2;

  for (y=0; y<image->h; y++) {
    x1 = x2 = -1;

    for (x=0; x<image->w; x++) {
      if (image_getpixel(image, x, y) != image_getpixel(image_diff, x, y)) {
	x1 = x;
	break;
      }
    }

    for (x=image->w-1; x>=0; x--) {
      if (image_getpixel(image, x, y) != image_getpixel(image_diff, x, y)) {
	x2 = x;
	break;
      }
    }

    if (x1 >= 0 && x2 >= 0)
      dirty_hline(dirty, x1, y, x2);
  }

  return dirty;
}

void dirty_free(Dirty* dirty)
{
  register DirtyRow* row;
  register DirtyCol* col;
  DirtyRow* rowend;
  DirtyCol* colend;

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

void dirty_putpixel(Dirty* dirty, int x, int y)
{
  DirtyRow* row;		/* row=dirty->row+v */
  DirtyCol* col;		/* col=row->col+u */
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
	!(dirty->mask->bitmap->getpixel(x-dirty->mask->x,
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

      col->data = jrealloc(col->data, dirty_line_size(dirty, col->w));
      return;
    }
    /* the pixel is to left of the column */
    else if (x+1 == col->x) {
      RESTORE_IMAGE(col);

      --col->x;
      ++col->w;
      col->data = jrealloc(col->data, dirty_line_size(dirty, col->w));

      col->ptr = image_address(dirty->image, x, y);
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
  col->data = jmalloc(dirty_line_size(dirty, 1));
  col->ptr = image_address(dirty->image, x, y);
}

void dirty_hline(Dirty* dirty, int x1, int y, int x2)
{
  DirtyRow* row;		/* row=dirty->row+v */
  DirtyCol* col;		/* col=row->col+u */
  int x, u, v, w;
  int u2, u3, to_x2;

  if (dirty->tiled) {
    if (x1 > x2)
      return;

    dirty->tiled = false;

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

    dirty->tiled = true;
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
	if (dirty->mask->bitmap->getpixel(x1-dirty->mask->x,
					  y-dirty->mask->y))
	  break;

      for (; x2>=x1; x2--)
	if (dirty->mask->bitmap->getpixel(x2-dirty->mask->x,
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
      if (!dirty->mask->bitmap->getpixel(x-dirty->mask->x,
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
		     dirty_line_size(dirty, (col+1)->w));
	    }

	    col->w += (col+1)->w;

	    /* remove the col[u+1] */
	    if ((col+1)->data)
	      jfree((col+1)->data);

	    if (u+1 < row->cols-1)
	      memmove(col+1, col+2,
		      sizeof(DirtyCol) * (row->cols-2-u));

	    row->cols--;
	    row->col = (DirtyCol*)jrealloc(row->col, sizeof(DirtyCol) * row->cols);
	    col = row->col+u;
	  }

	  col->data = jrealloc(col->data, dirty_line_size(dirty, col->w));
	  goto done;
	}
	/* the pixel is in the left of the column */
	else if (x == col->x-1) {
	  if (col->flags & DIRTY_VALID_COLUMN) {
	    col->flags ^= DIRTY_VALID_COLUMN;

	    memcpy(col->ptr,
		   col->data,
		   dirty_line_size(dirty, col->w));
	  }

	  --col->x;
	  ++col->w;
	  col->data = jrealloc(col->data, dirty_line_size(dirty, col->w));
	  col->ptr = image_address(dirty->image, x, y);
	  goto done;
	}
	else if (x < col->x)
	  break;
      }

      /* add a new column */
      ++row->cols;
      row->col = (DirtyCol*)jrealloc(row->col, sizeof(DirtyCol) * row->cols);
      col = row->col+u;

      if (u < row->cols-1)
	memmove(col+1, col, sizeof(DirtyCol) * (row->cols-1-u));

      col->x = x;
      col->w = 1;
      col->flags = 0;
      col->data = jmalloc(dirty_line_size(dirty, 1));
      col->ptr = image_address(dirty->image, x, y);

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
	col->ptr = image_address(dirty->image, x1, y);

	/* inside the column */
	if (x2 <= col->x+col->w-1) {
	  col->data = jrealloc(col->data, dirty_line_size(dirty, col->w));
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
    col->data = jmalloc(dirty_line_size(dirty, col->w));
    col->ptr = image_address(dirty->image, x1, y);
  }
}

void dirty_vline(Dirty* dirty, int x, int y1, int y2)
{
  dirty_line(dirty, x, y1, x, y2);
}

void dirty_line(Dirty* dirty, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, NULL, 0 };
  algo_line(x1, y1, x2, y2, &data, (AlgoPixel)algo_putpixel);
}

void dirty_rect(Dirty* dirty, int x1, int y1, int x2, int y2)
{
  dirty_hline(dirty, x1, y1, x2);
  dirty_hline(dirty, x1, y2, x2);
  dirty_vline(dirty, x1, y1, y2);
  dirty_vline(dirty, x2, y1, y2);
}

void dirty_rectfill(Dirty* dirty, int x1, int y1, int x2, int y2)
{
  int y;

  for (y=y1; y<=y2; ++y)
    dirty_hline(dirty, x1, y, x2);
}

void dirty_putpixel_pen(Dirty* dirty, Pen* pen, int x, int y)
{
  AlgoData data = { dirty, pen, 0 };
  if (pen->get_size() == 1)
    algo_putpixel(x, y, &data);
  else
    algo_putpen(x, y, &data);
}

void dirty_hline_pen(Dirty* dirty, Pen* pen, int x1, int y, int x2)
{
  AlgoData data = { dirty, pen, 0 };
  int x;

  if (pen->get_size() == 1)
    for (x=x1; x<=x2; ++x)
      algo_putpixel(x, y, &data);
  else
    for (x=x1; x<=x2; ++x)
      algo_putpen(x, y, &data);
}

void dirty_line_pen(Dirty* dirty, Pen* pen, int x1, int y1, int x2, int y2)
{
  AlgoData data = { dirty, pen, 0 };
  algo_line(x1, y1, x2, y2, &data,
	    (pen->get_size() == 1)?
	    (AlgoPixel)algo_putpixel:
	    (AlgoPixel)algo_putpen);
}

void dirty_save_image_data(Dirty* dirty)
{
  register int v, u, shift = image_shift(dirty->image);

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

void dirty_restore_image_data(Dirty* dirty)
{
  register DirtyRow* row;
  register DirtyCol* col;
  DirtyRow* rowend;
  DirtyCol* colend;
  int shift = image_shift(dirty->image);

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

void dirty_swap(Dirty* dirty)
{
  register DirtyRow* row;
  register DirtyCol* col;
  DirtyRow* rowend;
  DirtyCol* colend;
  HLineSwapper proc;

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

static void algo_putpen(int x, int y, AlgoData *data)
{
  register PenScanline* scanline = data->pen->get_scanline();
  register int c = data->pen->get_size()/2;

  x -= c;
  y -= c;

  for (c=0; c<data->pen->get_size(); ++c) {
    if (scanline->state)
      dirty_hline(data->dirty, x+scanline->x1, y+c, x+scanline->x2);
    ++scanline;
  }
}

static HLineSwapper swap_hline(Image* image)
{
  register HLineSwapper proc;
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
  register type* address1 = (type*)image;	\
  register type* address2 = (type*)data;	\
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

static void swap_hline32(void* image, void* data, int x1, int x2)
{
  SWAP_HLINE(ase_uint32);
}

static void swap_hline16(void* image, void* data, int x1, int x2)
{
  SWAP_HLINE(ase_uint16);
}

static void swap_hline8(void* image, void* data, int x1, int x2)
{
  SWAP_HLINE(ase_uint8);
}
