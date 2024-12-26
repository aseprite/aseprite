// Aseprite UI Library
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_GRID_H_INCLUDED
#define UI_GRID_H_INCLUDED
#pragma once

#include "ui/widget.h"

#include <vector>

namespace ui {

class Grid : public Widget {
public:
  struct Info {
    int col, row;
    int hspan, vspan;
    int grid_cols, grid_rows;
    Info() : col(0), row(0), hspan(0), vspan(0), grid_cols(0), grid_rows(0) {}
  };

  Grid(int columns, bool same_width_columns);
  ~Grid();

  void addChildInCell(Widget* child, int hspan, int vspan, int align);
  Info getChildInfo(Widget* child);
  void setGap(const gfx::Size& gap);
  void setStyle(Style* style) override;
  void setColumns(int columns);

protected:
  // Events
  void onResize(ResizeEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;

  // Separation between rows of grid cells. Negative values will overlap rows.
  int m_rowgap;
  // Separation between columns of grid cells. Negative values will overlap columns.
  int m_colgap;

private:
  struct Cell {
    Cell* parent;
    Widget* child;
    int hspan;
    int vspan;
    int align;
    int w, h;

    Cell();
  };

  struct Strip {
    int size;
    int expand_count;
  };

  void sumStripSize(const std::vector<Strip>& strip, int& size);
  void calculateCellSize(int start, int span, const std::vector<Strip>& strip, int& size);
  void calculateSize();
  void calculateStripSize(std::vector<Strip>& colstrip, std::vector<Strip>& rowstrip, int align);
  void expandStrip(std::vector<Strip>& colstrip,
                   std::vector<Strip>& rowstrip,
                   void (Grid::*incCol)(int, int));
  void distributeSize(const gfx::Rect& rect);
  void distributeStripSize(std::vector<Strip>& colstrip,
                           int rect_size,
                           int border_size,
                           bool same_width);
  bool putWidgetInCell(Widget* child, int hspan, int vspan, int align);
  void expandRows(int rows);
  void incColSize(int col, int size);
  void incRowSize(int row, int size);

  bool m_same_width_columns;
  std::vector<Strip> m_colstrip;
  std::vector<Strip> m_rowstrip;
  std::vector<std::vector<Cell*>> m_cells;
};

} // namespace ui

#endif
