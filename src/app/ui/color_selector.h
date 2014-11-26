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

#ifndef APP_UI_COLOR_SELECTOR_H_INCLUDED
#define APP_UI_COLOR_SELECTOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_sliders.h"
#include "app/ui/hex_color_entry.h"
#include "app/ui/palette_view.h"
#include "app/ui/popup_window_pin.h"
#include "base/connection.h"
#include "base/signal.h"
#include "ui/button.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/tooltips.h"
#include "ui/view.h"

namespace app {
  class PaletteIndexChangeEvent;

  class ColorSelector : public PopupWindowPin {
  public:
    enum SetColorOptions {
      ChangeType,
      DoNotChangeType
    };

    ColorSelector();
    ~ColorSelector();

    void setColor(const app::Color& color, SetColorOptions options);
    app::Color getColor() const;

    // Signals
    Signal1<void, const app::Color&> ColorChange;

  protected:
    void onColorPaletteIndexChange(PaletteIndexChangeEvent& ev);;
    void onColorSlidersChange(ColorSlidersChangeEvent& ev);
    void onColorHexEntryChange(const app::Color& color);
    void onColorTypeButtonClick(ui::Event& ev);
    void onFixWarningClick(ui::Event& ev);
    void onPaletteChange();

  private:
    void selectColorType(app::Color::Type type);
    void setColorWithSignal(const app::Color& color);
    void findBestfitIndex(const app::Color& color);

    class WarningIcon;

    ui::TooltipManager m_tooltips;
    ui::Box m_vbox;
    ui::Box m_topBox;
    app::Color m_color;
    ui::View m_colorPaletteContainer;
    PaletteView m_colorPalette;
    ui::RadioButton m_indexButton;
    ui::RadioButton m_rgbButton;
    ui::RadioButton m_hsvButton;
    ui::RadioButton m_grayButton;
    ui::RadioButton m_maskButton;
    HexColorEntry m_hexColorEntry;
    RgbSliders m_rgbSliders;
    HsvSliders m_hsvSliders;
    GraySlider m_graySlider;
    ui::Label m_maskLabel;
    WarningIcon* m_warningIcon;
    ScopedConnection m_onPaletteChangeConn;

    // This variable is used to avoid updating the m_hexColorEntry text
    // when the color change is generated from a
    // HexColorEntry::ColorChange signal. In this way we don't override
    // what the user is writting in the text field.
    bool m_disableHexUpdate;
  };

} // namespace app

#endif
