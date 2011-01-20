/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef WIDGETS_COLOR_BAR_H_INCLUDED
#define WIDGETS_COLOR_BAR_H_INCLUDED

#include "app/color.h"
#include "base/signal.h"
#include "gui/widget.h"

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

  Color getFgColor() const { return m_fgcolor; }
  Color getBgColor() const { return m_bgcolor; }
  void setFgColor(const Color& color);
  void setBgColor(const Color& color);

  Color getColorByPosition(int x, int y);

  // Signals
  Signal1<void, const Color&> FgColorChange;
  Signal1<void, const Color&> BgColorChange;

protected:
  bool onProcessMessage(JMessage msg);

private:
  int getEntriesCount() const {
    return m_columns*m_colorsPerColumn;
  }

  Color getEntryColor(int i) const {
    return Color::fromIndex(i+m_firstIndex);
  }

  Color getHotColor(hotcolor_t hot);
  void setHotColor(hotcolor_t hot, const Color& color);
  gfx::Rect getColumnBounds(int column) const;
  gfx::Rect getEntryBounds(int index) const;
  gfx::Rect getFgBounds() const;
  gfx::Rect getBgBounds() const;
  void updateStatusBar(const Color& color, int msecs);

  int m_firstIndex;
  int m_columns;
  int m_colorsPerColumn;
  int m_entrySize;
  Color m_fgcolor;
  Color m_bgcolor;
  hotcolor_t m_hot;
  hotcolor_t m_hot_editing;

  // Drag & drop colors
  hotcolor_t m_hot_drag;
  hotcolor_t m_hot_drop;

};

int colorbar_type();

#endif
