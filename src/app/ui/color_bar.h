/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_UI_COLOR_BAR_H_INCLUDED
#define APP_UI_COLOR_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_button.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/palette_popup.h"
#include "app/ui/palette_view.h"
#include "base/signal.h"
#include "base/unique_ptr.h"
#include "raster/pixel_format.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/view.h"

namespace app {
  class ColorButton;
  class PalettesLoader;

  class ColorBar : public ui::Box {
    static ColorBar* m_instance;
  public:
    static ColorBar* instance() { return m_instance; }

    ColorBar(int align);
    ~ColorBar();

    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getFgColor();
    app::Color getBgColor();
    void setFgColor(const app::Color& color);
    void setBgColor(const app::Color& color);

    PaletteView* getPaletteView();

    // Used by the Palette Editor command to change the status of button
    // when the visibility of the dialog changes.
    void setPaletteEditorButtonState(bool state);

    // Signals
    Signal1<void, const app::Color&> FgColorChange;
    Signal1<void, const app::Color&> BgColorChange;

  protected:
    void onPaletteButtonClick();
    void onPaletteButtonDropDownClick();
    void onPaletteIndexChange(int index);
    void onFgColorButtonChange(const app::Color& color);
    void onBgColorButtonChange(const app::Color& color);
    void onColorButtonChange(const app::Color& color);

  private:
    class ScrollableView : public ui::View {
    public:
      ScrollableView();
    protected:
      void onPaint(ui::PaintEvent& ev) override;
    };

    DropDownButton m_paletteButton;
    PalettePopup m_palettePopup;
    ScrollableView m_scrollableView;
    PaletteView m_paletteView;
    ColorButton m_fgColor;
    ColorButton m_bgColor;
    bool m_lock;
  };

} // namespace app

#endif
