/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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
 *   * Neither the name of the Jinete nor the names of its contributors may
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jwidget.h"

typedef struct Cell
{
  struct Cell *parent;
  JWidget child;
  int hspan;
  int vspan;
  int align;
} Cell;

typedef struct Strip
{
  int size;
  int align;
} Strip;

typedef struct Grid
{
  bool same_width_columns;
  int cols;
  int rows;
  Strip *colstrip;
  Strip *rowstrip;
  Cell **cells;
} Grid;

static bool grid_msg_proc(JWidget widget, JMessage msg);
static void grid_request_size(JWidget widget, int *w, int *h);
static void grid_set_position(JWidget widget, JRect rect);
static void grid_calculate_size(JWidget widget);
static void grid_distribute_size(JWidget widget, JRect rect);
static bool grid_put_in_a_cell(JWidget widget, JWidget child, int hspan, int vspan, int align);
static void grid_expand_rows(JWidget widget, int rows);

JWidget jgrid_new(int columns, bool same_width_columns)
{
  JWidget widget = jwidget_new(JI_GRID);
  Grid *grid = jnew(Grid, 1);
  int col;

  assert(columns > 0);

  grid->same_width_columns = same_width_columns;
  grid->cols = columns;
  grid->rows = 0;
  grid->colstrip = jmalloc(sizeof(Strip) * grid->cols);
  grid->rowstrip = NULL;
  grid->cells = NULL;

  for (col=0; col<grid->cols; ++col) {
    grid->colstrip[col].size = 0;
    grid->colstrip[col].align = 0;
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
  Grid *grid = jwidget_get_data(widget, JI_GRID);

  jwidget_add_child(widget, child);

  if (!grid_put_in_a_cell(widget, child, hspan, vspan, align)) {
    grid_expand_rows(widget, grid->rows+1);
    grid_put_in_a_cell(widget, child, hspan, vspan, align);
  }
}

static bool grid_msg_proc(JWidget widget, JMessage msg)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);

  switch (msg->type) {

    case JM_DESTROY:
      if (grid->cells != NULL) {
	int row;
	for (row=0; row<grid->rows; ++row)
	  jfree(grid->cells[row]);
	jfree(grid->cells);
      }
      jfree(grid);
      break;

    case JM_REQSIZE:
      grid_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS:
      grid_set_position(widget, &msg->setpos.rect);
      return TRUE;

#if 0				/* TODO */
    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_REMOVE_CHILD) {
      }
      break;
#endif

  }

  return FALSE;
}

static void grid_request_size(JWidget widget, int *w, int *h)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
  int i, j;

  grid_calculate_size(widget);

  /* calculate the total */

  *w = 0;
  *h = 0;

  for (i=j=0; i<grid->cols; ++i) {
    if (grid->colstrip[i].size > 0) {
      *w += grid->colstrip[i].size;
      if (++j > 1)
	*w += widget->child_spacing;
    }
  }

  for (i=j=0; i<grid->rows; ++i) {
    if (grid->rowstrip[i].size > 0) {
      *h += grid->rowstrip[i].size;
      if (++j > 1)
	*h += widget->child_spacing;
    }
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void grid_set_position(JWidget widget, JRect rect)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
  JRect cpos = jrect_new(0, 0, 0, 0);
  int pos_x, pos_y;
  int req_w, req_h;
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

#define CALCULATE_W(col, hspan, colstrip, w)		\
	for (i=col,j=0; i<col+cell->hspan; ++i) {	\
	  if (grid->colstrip[i].size > 0) {		\
	    w += grid->colstrip[i].size;		\
	    if (++j > 1)				\
	      w += widget->child_spacing;		\
	  }						\
	}

	CALCULATE_W(col, hspan, colstrip, w);
	CALCULATE_W(row, vspan, rowstrip, h);

	jwidget_request_size(cell->child, &req_w, &req_h);

	if (cell->align & JI_LEFT) {
	  w = req_w;
	}
	else if (cell->align & JI_CENTER) {
	  x += w/2 - req_w/2;
	  w = req_w;
	}
	else if (cell->align & JI_RIGHT) {
	  x += w - req_w;
	  w = req_w;
	}

	if (cell->align & JI_TOP) {
	  h = req_h;
	}
	else if (cell->align & JI_MIDDLE) {
	  y += h/2 - req_h/2;
	  h = req_h;
	}
	else if (cell->align & JI_BOTTOM) {
	  y += h - req_h;
	  h = req_h;
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
 * Calculates the size of each row and each column in the grid.
 */
static void grid_calculate_size(JWidget widget)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
  int row, col, size, align;
  int req_w, req_h;
  int max_w;
  Cell *cell;

  if (grid->rows == 0)
    return;
  
  for (row=0; row<grid->rows; ++row) {
    size = 0;
    align = 0;
    for (col=0; col<grid->cols; ++col) {
      cell = grid->cells[row]+col;
      req_w = req_h = 0;

      if (cell->child != NULL && cell->vspan == 1) {
	if (!(cell->child->flags & JI_HIDDEN)) {
	  jwidget_request_size(cell->child, &req_w, &req_h);
	  align |= cell->align;
	}

	col += cell->vspan-1;
      }

      size = MAX(size, req_h);
    }
    grid->rowstrip[row].size = size;
    grid->rowstrip[row].align = align;
  }

  for (col=0; col<grid->cols; ++col) {
    size = 0;
    align = 0;
    for (row=0; row<grid->rows; ++row) {
      cell = grid->cells[row]+col;
      req_w = req_h = 0;

      if (cell->child != NULL && cell->hspan == 1) {
	if (!(cell->child->flags & JI_HIDDEN)) {
	  jwidget_request_size(cell->child, &req_w, &req_h);
	  align |= cell->align;
	}

	row += cell->hspan-1;
      }

      size = MAX(size, req_w);
    }
    grid->colstrip[col].size = size;
    grid->colstrip[col].align = align;
  }

  /* same width in all columns */
  if (grid->same_width_columns) {
    max_w = 0;

    for (col=0; col<grid->cols; ++col)
      max_w = MAX(max_w, grid->colstrip[col].size);

    for (col=0; col<grid->cols; ++col)
      grid->colstrip[col].size = max_w;
  }
}

static void grid_distribute_size(JWidget widget, JRect rect)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
  int total_req, wantmore_count;
  int i, j;

  if (grid->rows == 0)
    return;

#define DISTRIBUTE_SIZE(cols, colstrip, JI_HORIZONTAL, l, r, jrect_w)	\
  total_req = 0;							\
  wantmore_count = 0;							\
  for (i=j=0; i<grid->cols; ++i) {					\
    if (grid->colstrip[i].size > 0) {					\
      total_req += grid->colstrip[i].size;				\
      if (++j > 1)							\
	total_req += widget->child_spacing;				\
    }									\
									\
    if (grid->colstrip[i].align & JI_HORIZONTAL ||			\
	grid->same_width_columns) {					\
      ++wantmore_count;							\
    }									\
  }									\
  total_req += widget->border_width.l + widget->border_width.r;		\
									\
  if (wantmore_count > 0) {						\
    int extra_total = jrect_w(rect) - total_req;			\
    if (extra_total > 0) {						\
      int extra_foreach = extra_total / wantmore_count;			\
									\
      for (i=0; i<grid->cols; ++i) {					\
	if (grid->colstrip[i].align & JI_HORIZONTAL ||			\
	    grid->same_width_columns) {					\
	  assert(wantmore_count > 0);					\
	  assert(extra_total > 0);					\
									\
	  grid->colstrip[i].size += extra_foreach;			\
	  extra_total -= extra_foreach;					\
									\
	  if (--wantmore_count == 0) {					\
	    grid->colstrip[i].size += extra_total;			\
	    extra_total = 0;						\
	  }								\
	}								\
      }									\
      assert(wantmore_count == 0);					\
      assert(extra_total == 0);						\
    }									\
  }

  DISTRIBUTE_SIZE(cols, colstrip, JI_HORIZONTAL, l, r, jrect_w);
  DISTRIBUTE_SIZE(rows, rowstrip, JI_VERTICAL,   t, b, jrect_h);
}

static bool grid_put_in_a_cell(JWidget widget, JWidget child, int hspan, int vspan, int align)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
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
	  /* hspan or vspan are bad specified (overlapping with other cells) */
	  assert(cell->parent == NULL);
	  assert(cell->child == NULL);

	  cell->parent = parentcell;
	  cell->child = child;
	}

	for (++row; row<rowend; ++row) {
	  cell = grid->cells[grid->cols*row + col];
	  for (col=colbeg; col<colend; ++col) {
	    assert(cell->parent == NULL);
	    assert(cell->child == NULL);

	    cell->parent = parentcell;
	    cell->child = child;
	  }
	}
	return TRUE;
      }
    }
  }

  return FALSE;
}

/**
 * Expands the grid's rows to reach the specified quantity of rows in
 * the parameter.
 */
static void grid_expand_rows(JWidget widget, int rows)
{
  Grid *grid = jwidget_get_data(widget, JI_GRID);
  Cell *cell;
  int row;

  if (grid->rows < rows) {
    grid->cells = jrealloc(grid->cells, sizeof(Cell *) * rows);
    grid->rowstrip = jrealloc(grid->rowstrip, sizeof(Strip) * rows);

    for (row=grid->rows; row<rows; ++row) {
      grid->cells[row] = jmalloc(sizeof(Cell) * grid->cols);
      grid->rowstrip[row].size = 0;
      grid->rowstrip[row].align = 0;

      for (cell=grid->cells[row];
	   cell<grid->cells[row]+grid->cols; ++cell) {
	cell->parent = NULL;
	cell->child = NULL;
	cell->hspan = 0;
	cell->vspan = 0;
      }
    }

    grid->rows = rows;
  }
}
