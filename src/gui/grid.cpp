// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/memory.h"
#include "gfx/size.h"
#include "gui/grid.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/preferred_size_event.h"
#include "gui/theme.h"
#include "gui/widget.h"

using namespace gfx;

Grid::Cell::Cell()
{
  parent = NULL;
  child = NULL;
  hspan = 0;
  vspan = 0;
  align = 0;
  w = 0;
  h = 0;
}

Grid::Grid(int columns, bool same_width_columns)
  : Widget(JI_GRID)
  , m_colstrip(columns)
{
  ASSERT(columns > 0);

  m_same_width_columns = same_width_columns;

  for (size_t col=0; col<m_colstrip.size(); ++col) {
    m_colstrip[col].size = 0;
    m_colstrip[col].expand_count = 0;
  }

  jwidget_init_theme(this);
}

Grid::~Grid()
{
  // Delete all cells.
  for (size_t row=0; row<m_cells.size(); ++row)
    for (size_t col=0; col<m_cells[row].size(); ++col)
      delete m_cells[row][col];
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
void Grid::addChildInCell(Widget* child, int hspan, int vspan, int align)
{
  ASSERT(hspan > 0);
  ASSERT(vspan > 0);

  jwidget_add_child(this, child);

  if (!putWidgetInCell(child, hspan, vspan, align)) {
    expandRows(m_rowstrip.size()+1);
    putWidgetInCell(child, hspan, vspan, align);
  }
}

bool Grid::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      setGridPosition(&msg->setpos.rect);
      return true;

    case JM_DRAW:
      getTheme()->draw_grid(this, &msg->draw.rect);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Grid::onPreferredSize(PreferredSizeEvent& ev)
{
  int w, h;

  w = h = 0;

  calculateSize();

  // Calculate the total
  sumStripSize(m_colstrip, w);
  sumStripSize(m_rowstrip, h);

  w += this->border_width.l + this->border_width.r;
  h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

void Grid::sumStripSize(const std::vector<Strip>& strip, int& size)
{
  int i, j;

  size = 0;
  for (i=j=0; i<(int)strip.size(); ++i) {
    if (strip[i].size > 0) {
      size += strip[i].size;
      if (++j > 1)
	size += this->child_spacing;
    }
  }
}

void Grid::setGridPosition(JRect rect)
{
  JRect cpos = jrect_new(0, 0, 0, 0);
  int pos_x, pos_y;
  Size reqSize;
  int x, y, w, h;
  int col, row;

  jrect_copy(this->rc, rect);

  calculateSize();
  distributeSize(rect);

  pos_y = rect->y1 + this->border_width.t;
  for (row=0; row<(int)m_rowstrip.size(); ++row) {
    pos_x = rect->x1 + this->border_width.l;

    for (col=0; col<(int)m_colstrip.size(); ++col) {
      Cell* cell = m_cells[row][col];

      if (cell->child != NULL &&
	  cell->parent == NULL &&
	  !(cell->child->flags & JI_HIDDEN)) {
	x = pos_x;
	y = pos_y;

	calculateCellSize(col, cell->hspan, m_colstrip, w);
	calculateCellSize(row, cell->vspan, m_rowstrip, h);

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

      if (m_colstrip[col].size > 0)
	pos_x += m_colstrip[col].size + this->child_spacing;
    }

    if (m_rowstrip[row].size > 0)
      pos_y += m_rowstrip[row].size + this->child_spacing;
  }

  jrect_free(cpos);
}

void Grid::calculateCellSize(int start, int span, const std::vector<Strip>& strip, int& size)
{
  int i, j;

  size = 0;

  for (i=start, j=0; i<start+span; ++i) {
    if (strip[i].size > 0) {
      size += strip[i].size;
      if (++j > 1)
	size += this->child_spacing;
    }
  }
}

// Calculates the size of each strip (rows and columns) in the grid.
void Grid::calculateSize()
{
  if (m_rowstrip.size() == 0)
    return;
  
  calculateStripSize(m_colstrip, m_rowstrip, JI_HORIZONTAL);
  calculateStripSize(m_rowstrip, m_colstrip, JI_VERTICAL);

  expandStrip(m_colstrip, m_rowstrip, &Grid::incColSize);
  expandStrip(m_rowstrip, m_colstrip, &Grid::incRowSize);

  // Same width in all columns
  if (m_same_width_columns) {
    int max_w = 0;
    for (int col=0; col<(int)m_colstrip.size(); ++col)
      max_w = MAX(max_w, m_colstrip[col].size);

    for (int col=0; col<(int)m_colstrip.size(); ++col)
      m_colstrip[col].size = max_w;
  }
}

void Grid::calculateStripSize(std::vector<Strip>& colstrip,
			      std::vector<Strip>& rowstrip, int align)
{
  Cell* cell;

  // For each column
  for (int col=0; col<(int)colstrip.size(); ++col) {
    // A counter of widgets that want more space in this column
    int expand_count = 0;

    // For each row
    for (int row=0; row<(int)rowstrip.size(); ++row) {
      // For each cell
      if (&colstrip == &m_colstrip)
	cell = m_cells[row][col];
      else
	cell = m_cells[col][row]; // Transposed

      if (cell->child != NULL) {
	if (cell->parent == NULL) {
	  // If the widget isn't hidden then we can request its size
	  if (!(cell->child->flags & JI_HIDDEN)) {
	    Size reqSize = cell->child->getPreferredSize();
	    cell->w = reqSize.w - (cell->hspan-1) * this->child_spacing;
	    cell->h = reqSize.h - (cell->vspan-1) * this->child_spacing;

	    if ((cell->align & align) == align)
	      ++expand_count;
	  }
	  else
	    cell->w = cell->h = 0;
	}
	else {
	  if (!(cell->child->flags & JI_HIDDEN)) {
	    if ((cell->parent->align & align) == align)
	      ++expand_count;
	  }
	}

	if (&colstrip == &m_colstrip)
	  row += cell->vspan-1;
	else
	  row += cell->hspan-1;	// Transposed
      }
    }

    colstrip[col].size = 0;
    colstrip[col].expand_count = expand_count;
  }
}

void Grid::expandStrip(std::vector<Strip>& colstrip,
		       std::vector<Strip>& rowstrip,
		       void (Grid::*incCol)(int, int))
{
  bool more_span;
  int i, current_span = 1;

  do {
    more_span = false;
    for (int col=0; col<(int)colstrip.size(); ++col) {
      for (int row=0; row<(int)rowstrip.size(); ++row) {
	int cell_size;
	int cell_span;
	Cell* cell;

	// For each cell
	if (&colstrip == &m_colstrip) {
	  cell = m_cells[row][col];
	  cell_size = cell->w;
	  cell_span = cell->hspan;
	}
	else {
	  cell = m_cells[col][row]; // Transposed
	  cell_size = cell->h;
	  cell_span = cell->vspan;
	}

	if (cell->child != NULL &&
	    cell->parent == NULL &&
	    cell_size > 0) {
	  ASSERT(cell_span > 0);

	  if (cell_span == current_span) {
	    // Calculate the maximum (expand_count) in cell's columns.
	    int max_expand_count = 0;
	    for (i=col; i<col+cell_span; ++i)
	      max_expand_count = MAX(max_expand_count,
				     colstrip[i].expand_count);

	    int expand = 0;	// How many columns have the maximum value of "expand_count"
	    int last_expand = 0; // This variable is used to add the remainder space to the last column
	    for (i=col; i<col+cell_span; ++i) {
	      if (colstrip[i].expand_count == max_expand_count) {
		++expand;
		last_expand = i;
	      }
	    }

	    // Divide the available size of the cell in the number of columns which are expandible
	    int size = cell_size / expand;
	    for (i=col; i<col+cell_span; ++i) {
	      if (colstrip[i].expand_count == max_expand_count) {
		// For the last column, use all the available space in the column
		if (last_expand == i) {
		  if (&colstrip == &m_colstrip)
		    size = cell->w;
		  else
		    size = cell->h; // Transposed
		}

		(this->*incCol)(i, size);
	      }
	    }
	  }
	  else if (cell_span > current_span) {
	    more_span = true;
	  }
	}
      }
    }
    ++current_span;
  } while (more_span);
}

void Grid::distributeSize(JRect rect)
{
  if (m_rowstrip.size() == 0)
    return;

  distributeStripSize(m_colstrip, jrect_w(rect), this->border_width.l + this->border_width.r, m_same_width_columns);
  distributeStripSize(m_rowstrip, jrect_h(rect), this->border_width.t + this->border_width.b, false);
}

void Grid::distributeStripSize(std::vector<Strip>& colstrip, 
			       int rect_size, int border_size, bool same_width)
{
  int i, j;

  int max_expand_count = 0;
  for (i=0; i<(int)colstrip.size(); ++i)
    max_expand_count = MAX(max_expand_count,
			   colstrip[i].expand_count);

  int total_req = 0;
  int wantmore_count = 0;
  for (i=j=0; i<(int)colstrip.size(); ++i) {
    if (colstrip[i].size > 0) {
      total_req += colstrip[i].size;
      if (++j > 1)
	total_req += this->child_spacing;
    }

    if (colstrip[i].expand_count == max_expand_count || same_width) {
      ++wantmore_count;
    }
  }
  total_req += border_size;

  if (wantmore_count > 0) {
    int extra_total = rect_size - total_req;
    if (extra_total > 0) {
      // If a expandable column-strip was empty (size=0) then we have
      // to reduce the extra_total size because a new child-spacing is
      // added by this column
      for (i=0; i<(int)colstrip.size(); ++i) {
	if ((colstrip[i].size == 0) &&
	    (colstrip[i].expand_count == max_expand_count || same_width)) {
	  extra_total -= this->child_spacing;
	}
      }

      int extra_foreach = extra_total / wantmore_count;

      for (i=0; i<(int)colstrip.size(); ++i) {
	if (colstrip[i].expand_count == max_expand_count || same_width) {
	  ASSERT(wantmore_count > 0);

	  colstrip[i].size += extra_foreach;
	  extra_total -= extra_foreach;

	  if (--wantmore_count == 0) {
	    colstrip[i].size += extra_total;
	    extra_total = 0;
	  }
	}
      }

      ASSERT(wantmore_count == 0);
      ASSERT(extra_total == 0);
    }
  }
}

bool Grid::putWidgetInCell(Widget* child, int hspan, int vspan, int align)
{
  int col, row, colbeg, colend, rowend;
  Cell *cell, *parentcell;

  for (row=0; row<(int)m_rowstrip.size(); ++row) {
    for (col=0; col<(int)m_colstrip.size(); ++col) {
      cell = m_cells[row][col];

      if (cell->child == NULL) {
	cell->child = child;
	cell->hspan = hspan;
	cell->vspan = vspan;
	cell->align = align;

	parentcell = cell;
	colbeg = col;
	colend = MIN(col+hspan, (int)m_colstrip.size());
	rowend = row+vspan;

	expandRows(row+vspan);

	for (++col; col<colend; ++col) {
	  cell = m_cells[row][col];

	  // If these asserts fails, it's really possible that you
	  // specified bad values for hspan or vspan (they are
	  // overlapping with other cells).
	  ASSERT(cell->parent == NULL);
	  ASSERT(cell->child == NULL);

	  cell->parent = parentcell;
	  cell->child = child;
	  cell->hspan = colend - col;
	  cell->vspan = rowend - row;
	}

	for (++row; row<rowend; ++row) {
	  for (col=colbeg; col<colend; ++col) {
	    cell = m_cells[row][col];

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

// Expands the grid's rows to reach the specified quantity of rows in
// the parameter.
void Grid::expandRows(int rows)
{
  if ((int)m_rowstrip.size() < rows) {
    int old_size = (int)m_rowstrip.size();

    m_cells.resize(rows);
    m_rowstrip.resize(rows);

    for (int row=old_size; row<rows; ++row) {
      m_cells[row].resize(m_colstrip.size());
      m_rowstrip[row].size = 0;
      m_rowstrip[row].expand_count = 0;

      for (int col=0; col<(int)m_cells[row].size(); ++col) {
	m_cells[row][col] = new Cell;
      }
    }
  }
}

void Grid::incColSize(int col, int size)
{
  m_colstrip[col].size += size;

  for (int row=0; row<(int)m_rowstrip.size(); ) {
    Cell* cell = m_cells[row][col];

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

void Grid::incRowSize(int row, int size)
{
  m_rowstrip[row].size += size;

  for (int col=0; col<(int)m_colstrip.size(); ) {
    Cell* cell = m_cells[row][col];

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
