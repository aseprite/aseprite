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
#include "gui/box.h"
#include "gui/button.h"
#include "gui/view.h"
#include "widgets/color_button.h"
#include "widgets/palette_view.h"

class PaletteView;
class ColorButton;

class ColorBar : public Box
{
public:
  ColorBar(int align);
  ~ColorBar();

  void setImgType(int imgtype);

  Color getFgColor();
  Color getBgColor();
  void setFgColor(const Color& color);
  void setBgColor(const Color& color);

  PaletteView* getPaletteView();

  // Signals
  Signal1<void, const Color&> FgColorChange;
  Signal1<void, const Color&> BgColorChange;

protected:
  void onPaletteButtonClick();
  void onPaletteIndexChange(int index);
  void onFgColorButtonChange(const Color& color);
  void onBgColorButtonChange(const Color& color);
  void onColorButtonChange(const Color& color);

private:
  class ScrollableView : public View
  {
  public:
    ScrollableView();
  protected:
    bool onProcessMessage(JMessage msg);
  };

  Button m_paletteButton;
  ScrollableView m_scrollableView;
  PaletteView m_paletteView;
  ColorButton m_fgColor;
  ColorButton m_bgColor;
};

#endif
