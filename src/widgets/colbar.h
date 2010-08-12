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

#ifndef WIDGETS_COLBAR_H_INCLUDED
#define WIDGETS_COLBAR_H_INCLUDED

#include "Vaca/Signal.h"
#include "jinete/jwidget.h"

#include "core/color.h"

class ColorBar : public Widget
{
  typedef enum {
    HOTCOLOR_NONE = -3,
    HOTCOLOR_FGCOLOR = -2,
    HOTCOLOR_BGCOLOR = -1,
  } hotcolor_t;

public:
  ColorBar(int align);
  ~ColorBar();

  color_t getFgColor() const { return m_fgcolor; }
  color_t getBgColor() const { return m_bgcolor; }
  void setFgColor(color_t color);
  void setBgColor(color_t color);

  color_t getColorByPosition(int x, int y);

  // Signals
  Vaca::Signal1<void, color_t> FgColorChange;
  Vaca::Signal1<void, color_t> BgColorChange;

protected:
  bool onProcessMessage(JMessage msg);

private:
  int getEntriesCount() const { return m_columns*m_colorsPerColumn; }
  color_t getEntryColor(int i) const { return color_index(i+m_firstIndex); }

  color_t getHotColor(hotcolor_t hot);
  void setHotColor(hotcolor_t hot, color_t color);
  Rect getColumnBounds(int column) const;
  Rect getEntryBounds(int index) const;
  Rect getFgBounds() const;
  Rect getBgBounds() const;
  void updateStatusBar(color_t color, int msecs);

  int m_firstIndex;
  int m_columns;
  int m_colorsPerColumn;
  int m_entrySize;
  color_t m_fgcolor;
  color_t m_bgcolor;
  hotcolor_t m_hot;
  hotcolor_t m_hot_editing;

  // Drag & drop colors
  hotcolor_t m_hot_drag;
  hotcolor_t m_hot_drop;

};

int colorbar_type();

#endif
