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

#ifndef WIDGETS_PALEDIT_H_INCLUDED
#define WIDGETS_PALEDIT_H_INCLUDED

#include <allegro/color.h>

#include "jinete/jwidget.h"

// TODO use some JI_SIGNAL_USER
#define SIGNAL_PALETTE_EDITOR_CHANGE   0x10005

enum {
  PALETTE_EDITOR_RANGE_NONE,
  PALETTE_EDITOR_RANGE_LINEAL,
  PALETTE_EDITOR_RANGE_RECTANGULAR,
};

class PalEdit : public Widget
{
public:
  PalEdit(bool editable);

  int getRangeType();

  int getColumns();
  void setColumns(int columns);
  void setBoxSize(int boxsize);

  void selectColor(int index);
  void selectRange(int begin, int end, int range_type);

  void moveSelection(int x, int y);

  int get1stColor();
  int get2ndColor();
  void getSelectedEntries(bool array[256]);

protected:
  bool onProcessMessage(JMessage msg);

private:
  void request_size(int* w, int* h);
  void update_scroll(int color);

  bool m_editable;
  unsigned m_range_type;
  unsigned m_columns;
  int m_boxsize;
  int m_color[2];
};

#endif
