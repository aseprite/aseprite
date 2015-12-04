// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_SELECTOR_H_INCLUDED
#define APP_UI_COLOR_SELECTOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/button_set.h"
#include "app/ui/color_sliders.h"
#include "app/ui/hex_color_entry.h"
#include "app/ui/palette_view.h"
#include "app/ui/popup_window_pin.h"
#include "base/connection.h"
#include "base/signal.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/view.h"

namespace app {
  class PaletteIndexChangeEvent;

  class ColorSelector : public PopupWindowPin
                      , public PaletteViewDelegate {
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
    base::Signal1<void, const app::Color&> ColorChange;

  protected:
    void onColorSlidersChange(ColorSlidersChangeEvent& ev);
    void onColorHexEntryChange(const app::Color& color);
    void onColorTypeClick();
    void onPaletteChange();

    // PaletteViewDelegate impl
    void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) override;

  private:
    void selectColorType(app::Color::Type type);
    void setColorWithSignal(const app::Color& color);
    void findBestfitIndex(const app::Color& color);

    ui::Box m_vbox;
    ui::Box m_topBox;
    app::Color m_color;
    ui::View m_colorPaletteContainer;
    PaletteView m_colorPalette;
    ButtonSet m_colorType;
    HexColorEntry m_hexColorEntry;
    RgbSliders m_rgbSliders;
    HsvSliders m_hsvSliders;
    GraySlider m_graySlider;
    ui::Label m_maskLabel;
    base::ScopedConnection m_onPaletteChangeConn;

    // This variable is used to avoid updating the m_hexColorEntry text
    // when the color change is generated from a
    // HexColorEntry::ColorChange signal. In this way we don't override
    // what the user is writting in the text field.
    bool m_disableHexUpdate;
  };

} // namespace app

#endif
