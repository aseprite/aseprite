/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef WIDGETS_PALETTE_VIEW_H_INCLUDED
#define WIDGETS_PALETTE_VIEW_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/widget.h"

#include <allegro/color.h>
#include <vector>

class PaletteView : public ui::Widget
{
public:
  typedef std::vector<bool> SelectedEntries;

  PaletteView(bool editable);

  int getColumns() const { return m_columns; }
  void setColumns(int columns);
  void setBoxSize(int boxsize);

  void clearSelection();
  void selectColor(int index);
  void selectRange(int index1, int index2);

  int getSelectedEntry() const;
  bool getSelectedRange(int& index1, int& index2) const;
  void getSelectedEntries(SelectedEntries& entries) const;

  Color getColorByPosition(int x, int y);

  // Signals
  Signal1<void, int> IndexChange;

protected:
  bool onProcessMessage(ui::Message* msg) OVERRIDE;

private:
  void request_size(int* w, int* h);
  void update_scroll(int color);

  bool m_editable;
  int m_columns;
  int m_boxsize;
  int m_currentEntry;
  int m_rangeAnchor;
  SelectedEntries m_selectedEntries;
  bool m_isUpdatingColumns;
};

int palette_view_type();

#endif
