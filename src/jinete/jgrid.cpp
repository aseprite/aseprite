/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jwidget.h"
#include "jinete/jtheme.h"
#include "Vaca/Size.h"

struct Cell
{
  struct Cell *parent;
  JWidget child;
  int hspan;
  int vspan;
  int align;
  int w, h;
};

struct Strip
{
  int size;
  int expand_count;
};

struct Grid
{
  bool same_width_columns;
  int cols;
  int rows;
  Strip *colstrip;
  Strip *rowstrip;
  Cell **cells;
};

static bool grid_msg_proc(JWidget widget, JMessage msg);
static void grid_request_size(JWidget widget, int *w, int *h);
static void grid_set_position(JWidget widget, JRect rect);
static void grid_calculate_size(JWidget widget);
static void grid_distribute_size(JWidget widget, JRect rect);
static bool grid_put_widget_in_cell(JWidget widget, JWidget child, int hspan, int vspan, int align);
static void grid_expand_rows(JWidget widget, int rows);
static void grid_inc_col_size(Grid *grid, int col, int size);
static void grid_inc_row_size(Grid *grid, int row, int size);

JWidget jgrid_new(int columns, bool same_width_columns)
{
  Widget* widget = new Widget(JI_GRID);
  Grid *grid = jnew(Grid, 1);
  int col;

  ASSERT(columns > 0);

  grid->same_width_columns = same_width_columns;
  grid->cols = columns;
  grid->rows = 0;
  grid->colstrip = (Strip*)jmalloc(sizeof(Strip) * grid->cols);
  grid->rowstrip = NULL;
  grid->cells = NULL;

  for (col=0; col<grid->cols; ++col) {
    grid->colstrip[col].size = 0;
    grid->colstrip[col].expand_count = 0;
  }

  jwidget_add_hook(widget, JI_GRID, grid_msg_proc, grid);
  jwidget_init_theme(widget);

  return widget;
}

/**
 * Adds a child widget in the specified grid.
 * 
 * @param widget The grid widget.
 * @param child The child widget.
 * @param hspan
 * @param vspan
 * @param align
 * It's a combination of the following values:
 * 
 * - JI_HORIZONTAL: The widget'll get excess horizontal space.
 * - JI_VERTICAL: The widget'll get excess vertical space.
 *
 * - JI_LEFT: Sets horizontal alignment to the beginning of cell.
 * - JI_CENTER: Sets horizontal alignment to the center of cell.
 * - JI_RIGHT: Sets horizontal alignment to the end of cell.
 * - None: Uses the whole horizontal space of the cell.
 *
 * - JI_TOP: Sets vertical alignment to the beginning of the cell.
 * - JI_MIDDLE: Sets vertical alignment to the center of the cell.
 * - JI_BOTTOM: Sets vertical alignment to the end of the cell.
 * - None: Uses the whole vertical space of the cell.
 */
void jgrid_add_child(JWidget widget, JWidget child,
		     int hspan, int vspan, int align)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));

  ASSERT(hspan > 0);
  ASSERT(vspan > 0);

  jwidget_add_child(widget, child);

  if (!grid_put_widget_in_cell(widget, child, hspan, vspan, align)) {
    grid_expand_rows(widget, grid->rows+1);
    grid_put_widget_in_cell(widget, child, hspan, vspan, align);
  }
}

static bool grid_msg_proc(JWidget widget, JMessage msg)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));

  switch (msg->type) {

    case JM_DESTROY:
      if (grid->cells != NULL) {
	int row;
	for (row=0; row<grid->rows; ++row)
	  jfree(grid->cells[row]);
	jfree(grid->cells);
      }

      if (grid->colstrip != NULL)
	jfree(grid->colstrip);

      if (grid->rowstrip != NULL)
	jfree(grid->rowstrip);

      jfree(grid);
      break;

    case JM_REQSIZE:
      grid_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      grid_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      widget->theme->draw_grid(widget, &msg->draw.rect);
      return true;

  }

  return false;
}

static void grid_request_size(JWidget widget, int *w, int *h)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  int i, j;

  grid_calculate_size(widget);

  /* calculate the total */

#define SUMSTRIPS(p_cols, p_colstrip, p_w)	\
  *p_w = 0;					\
  for (i=j=0; i<grid->p_cols; ++i) {		\
    if (grid->p_colstrip[i].size > 0) {		\
      *p_w += grid->p_colstrip[i].size;		\
      if (++j > 1)				\
	*p_w += widget->child_spacing;		\
    }						\
  }

  SUMSTRIPS(cols, colstrip, w);
  SUMSTRIPS(rows, rowstrip, h);

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void grid_set_position(JWidget widget, JRect rect)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  JRect cpos = jrect_new(0, 0, 0, 0);
  int pos_x, pos_y;
  Size reqSize;
  int x, y, w, h;
  int col, row;
  int i, j;
  Cell *cell;

  jrect_copy(widget->rc, rect);

  grid_calculate_size(widget);
  grid_distribute_size(widget, rect);

  pos_y = rect->y1 + widget->border_width.t;
  for (row=0; row<grid->rows; ++row) {
    pos_x = rect->x1 + widget->border_width.l;

    cell = grid->cells[row];
    for (col=0; col<grid->cols; ++col, ++cell) {
      if (cell->child != NULL &&
	  cell->parent == NULL &&
	  !(cell->child->flags & JI_HIDDEN)) {
	x = pos_x;
	y = pos_y;
	w = 0;
	h = 0;

#define CALCULATE_CELL_SIZE(p_col, p_span, p_colstrip, p_w)	\
	for (i=p_col,j=0; i<p_col+cell->p_span; ++i) {		\
	  if (grid->p_colstrip[i].size > 0) {			\
	    p_w += grid->p_colstrip[i].size;			\
	    if (++j > 1)					\
	      p_w += widget->child_spacing;			\
	  }							\
	}

	CALCULATE_CELL_SIZE(col, hspan, colstrip, w);
	CALCULATE_CELL_SIZE(row, vspan, rowstrip, h);

	reqSize = cell->child->getPreferredSize();

	if (cell->align & JI_LEFT) {
	  w = reqSize.w;
	}
	else if (cell->align & JI_CENTER) {
	  x += w/2 - reqSize.w/2;
	  w = reqSize.w;
	}
	else if (cell->align & JI_RIGHT) {
	  x += w - reqSize.w;
	  w = reqSize.w;
	}

	if (cell->align & JI_TOP) {
	  h = reqSize.h;
	}
	else if (cell->align & JI_MIDDLE) {
	  y += h/2 - reqSize.h/2;
	  h = reqSize.h;
	}
	else if (cell->align & JI_BOTTOM) {
	  y += h - reqSize.h;
	  h = reqSize.h;
	}

	jrect_replace(cpos, x, y, x+w, y+h);
	jwidget_set_rect(cell->child, cpos);
      }

      if (grid->colstrip[col].size > 0)
	pos_x += grid->colstrip[col].size + widget->child_spacing;
    }

    if (grid->rowstrip[row].size > 0)
      pos_y += grid->rowstrip[row].size + widget->child_spacing;
  }

  jrect_free(cpos);
}

/**
 * Calculates the size of each strip (rows and columns) in the grid.
 */
static void grid_calculate_size(JWidget widget)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  int row, col, size;
  Size reqSize;
  int i, expand;
  int expand_count;
  int last_expand;
  int current_span;
  bool more_span;
  Cell *cell;

  if (grid->rows == 0)
    return;

#define CALCULATE_STRIPS(p_col, p_cols, p_row, p_rows, p_span,		\
			 p_req, p_colstrip, p_align)			\
  /* for each column */							\
  for (p_col=0; p_col<grid->p_cols; ++p_col) {				\
    /* a counter of widgets that want more space in this column */	\
    expand_count = 0;							\
    /* for each row */							\
    for (p_row=0; p_row<grid->p_rows; ++p_row) {			\
      /* for each cell */						\
      cell = grid->cells[row]+col;					\
      reqSize.w = reqSize.h = 0;					\
									\
      if (cell->child != NULL) {					\
	if (cell->parent == NULL) {					\
	  /* if the widget isn't hidden then we can request its size */ \
	  if (!(cell->child->flags & JI_HIDDEN)) {			\
	    reqSize = cell->child->getPreferredSize();			\
	    cell->w = reqSize.w - (cell->hspan-1) * widget->child_spacing;	\
	    cell->h = reqSize.h - (cell->vspan-1) * widget->child_spacing;	\
	    if ((cell->align & p_align) == p_align)			\
	      ++expand_count;						\
	  }								\
	  else								\
	    cell->w = cell->h = 0;					\
	}								\
	else {								\
	  if (!(cell->child->flags & JI_HIDDEN)) {			\
	    if ((cell->parent->align & p_align) == p_align)		\
	      ++expand_count;						\
	  }								\
	}								\
	p_row += cell->p_span-1;					\
      }									\
    }									\
    grid->p_colstrip[p_col].size = 0;					\
    grid->p_colstrip[p_col].expand_count = expand_count;		\
  }

#define EXPAND_STRIPS(p_col, p_cols, p_row, p_rows, p_span,		\
		      p_size, p_colstrip, p_inc_col)			\
  current_span = 1;							\
  do {									\
    more_span = false;							\
    for (p_col=0; p_col<grid->p_cols; ++p_col) {			\
      for (p_row=0; p_row<grid->p_rows; ++p_row) {			\
	cell = grid->cells[row]+col;					\
									\
	if (cell->child != NULL && cell->parent == NULL &&		\
	    cell->p_size > 0) {						\
	  ASSERT(cell->p_span > 0);					\
									\
	  if (cell->p_span == current_span) {				\
	    expand = 0;							\
	    expand_count = 0;						\
	    last_expand = 0;						\
									\
	    for (i=p_col; i<p_col+cell->p_span; ++i)			\
	      expand_count = MAX(expand_count,				\
				 grid->p_colstrip[i].expand_count);	\
									\
	    for (i=p_col; i<p_col+cell->p_span; ++i) {			\
	      if (grid->p_colstrip[i].expand_count == expand_count) {	\
		++expand;						\
		last_expand = i;					\
	      }								\
	    }								\
									\
	    size = cell->p_size / expand;				\
									\
	    for (i=p_col; i<p_col+cell->p_span; ++i) {			\
	      if (grid->p_colstrip[i].expand_count == expand_count) {	\
		if (last_expand == i)					\
		  size = cell->p_size;					\
									\
		p_inc_col(grid, i, size);				\
	      }								\
	    }								\
	  }								\
	  else if (cell->p_span > current_span) {			\
	    more_span = true;						\
	  }								\
	}								\
      }									\
    }									\
    ++current_span;							\
  } while (more_span);
  
  CALCULATE_STRIPS(col, cols, row, rows, vspan, reqSize.w, colstrip, JI_HORIZONTAL);
  CALCULATE_STRIPS(row, rows, col, cols, hspan, reqSize.h, rowstrip, JI_VERTICAL);
  EXPAND_STRIPS(col, cols, row, rows, hspan, w, colstrip, grid_inc_col_size);
  EXPAND_STRIPS(row, rows, col, cols, vspan, h, rowstrip, grid_inc_row_size);

  /* same width in all columns */
  if (grid->same_width_columns) {
    int max_w = 0;
    for (col=0; col<grid->cols; ++col)
      max_w = MAX(max_w, grid->colstrip[col].size);

    for (col=0; col<grid->cols; ++col)
      grid->colstrip[col].size = max_w;
  }
}

static void grid_distribute_size(JWidget widget, JRect rect)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  int total_req, expand_count, wantmore_count;
  int i, j;
  int extra_total;
  int extra_foreach;

  if (grid->rows == 0)
    return;

#define DISTRIBUTE_SIZE(p_cols, p_colstrip, p_l, p_r,			\
			p_jrect_w, p_same_width)			\
  expand_count = 0;							\
  for (i=0; i<grid->p_cols; ++i)					\
    expand_count = MAX(expand_count,					\
		       grid->p_colstrip[i].expand_count);		\
									\
  total_req = 0;							\
  wantmore_count = 0;							\
  for (i=j=0; i<grid->p_cols; ++i) {					\
    if (grid->p_colstrip[i].size > 0) {					\
      total_req += grid->p_colstrip[i].size;				\
      if (++j > 1)							\
	total_req += widget->child_spacing;				\
    }									\
    									\
    if (grid->p_colstrip[i].expand_count == expand_count ||		\
	p_same_width) {							\
      ++wantmore_count;							\
    }									\
  }									\
  total_req += widget->border_width.p_l + widget->border_width.p_r;	\
									\
  if (wantmore_count > 0) {						\
    extra_total = p_jrect_w(rect) - total_req;				\
    if (extra_total > 0) {						\
      /* if a expandable column-strip was empty (size=0) then */	\
      /* we have to reduce the extra_total size because a new */	\
      /* child-spacing is added by this column */			\
      for (i=0; i<grid->p_cols; ++i) {					\
	if ((grid->p_colstrip[i].size == 0) &&				\
	    (grid->p_colstrip[i].expand_count == expand_count ||	\
	     p_same_width)) {						\
	  extra_total -= widget->child_spacing;				\
	}								\
      }									\
									\
      extra_foreach = extra_total / wantmore_count;			\
									\
      for (i=0; i<grid->p_cols; ++i) {					\
	if (grid->p_colstrip[i].expand_count == expand_count ||		\
	    p_same_width) {						\
	  ASSERT(wantmore_count > 0);					\
									\
	  grid->p_colstrip[i].size += extra_foreach;			\
	  extra_total -= extra_foreach;					\
									\
	  if (--wantmore_count == 0) {					\
	    grid->p_colstrip[i].size += extra_total;			\
	    extra_total = 0;						\
	  }								\
	}								\
      }									\
      ASSERT(wantmore_count == 0);					\
      ASSERT(extra_total == 0);						\
    }									\
  }

  DISTRIBUTE_SIZE(cols, colstrip, l, r, jrect_w, grid->same_width_columns);
  DISTRIBUTE_SIZE(rows, rowstrip, t, b, jrect_h, false);
}

static bool grid_put_widget_in_cell(JWidget widget, JWidget child, int hspan, int vspan, int align)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  int col, row, colbeg, colend, rowend;
  Cell *cell, *parentcell;

  for (row=0; row<grid->rows; ++row) {
    cell = grid->cells[row];
    for (col=0; col<grid->cols; ++col, ++cell) {
      if (cell->child == NULL) {
	cell->child = child;
	cell->hspan = hspan;
	cell->vspan = vspan;
	cell->align = align;

	parentcell = cell;
	colbeg = col;
	colend = MIN(col+hspan, grid->cols);
	rowend = row+vspan;

	grid_expand_rows(widget, row+vspan);

	for (++col, ++cell; col<colend; ++col, ++cell) {
	  /* if these asserts fails, it's really possible that you
	     specified bad values for hspan or vspan (they are
	     overlapping with other cells) */
	  ASSERT(cell->parent == NULL);
	  ASSERT(cell->child == NULL);

	  cell->parent = parentcell;
	  cell->child = child;
	  cell->hspan = colend - col;
	  cell->vspan = rowend - row;
	}

	for (++row; row<rowend; ++row) {
	  for (col=colbeg; col<colend; ++col) {
	    cell = grid->cells[row]+col;

	    ASSERT(cell->parent == NULL);
	    ASSERT(cell->child == NULL);

	    cell->parent = parentcell;
	    cell->child = child;
	    cell->hspan = colend - col;
	    cell->vspan = rowend - row;
	  }
	}
	return true;
      }
    }
  }

  return false;
}

/**
 * Expands the grid's rows to reach the specified quantity of rows in
 * the parameter.
 */
static void grid_expand_rows(JWidget widget, int rows)
{
  Grid* grid = reinterpret_cast<Grid*>(jwidget_get_data(widget, JI_GRID));
  Cell* cell;
  int row;

  if (grid->rows < rows) {
    grid->cells = (Cell**)jrealloc(grid->cells, sizeof(Cell*) * rows);
    grid->rowstrip = (Strip*)jrealloc(grid->rowstrip, sizeof(Strip) * rows);

    for (row=grid->rows; row<rows; ++row) {
      grid->cells[row] = (Cell*)jmalloc(sizeof(Cell)*grid->cols);
      grid->rowstrip[row].size = 0;
      grid->rowstrip[row].expand_count = 0;

      for (cell=grid->cells[row];
	   cell<grid->cells[row]+grid->cols; ++cell) {
	cell->parent = NULL;
	cell->child = NULL;
	cell->hspan = 0;
	cell->vspan = 0;
	cell->align = 0;
	cell->w = 0;
	cell->h = 0;
      }
    }

    grid->rows = rows;
  }
}

static void grid_inc_col_size(Grid *grid, int col, int size)
{
  Cell *cell;
  int row;

  grid->colstrip[col].size += size;

  for (row=0; row<grid->rows; ) {
    cell = grid->cells[row]+col;

    if (cell->child != NULL) {
      if (cell->parent != NULL)
	cell->parent->w -= size;
      else
	cell->w -= size;

      row += cell->vspan;
    }
    else
      ++row;
  }
}

static void grid_inc_row_size(Grid *grid, int row, int size)
{
  Cell *cell;
  int col;

  grid->rowstrip[row].size += size;

  for (col=0; col<grid->cols; ) {
    cell = grid->cells[row]+col;

    if (cell->child != NULL) {
      if (cell->parent != NULL)
	cell->parent->h -= size;
      else
	cell->h -= size;

      col += cell->hspan;
    }
    else
      ++col;
  }
}
