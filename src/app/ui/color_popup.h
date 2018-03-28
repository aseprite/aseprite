// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_POPUP_H_INCLUDED
#define APP_UI_COLOR_POPUP_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/shade.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button_options.h"
#include "app/ui/color_shades.h"
#include "app/ui/color_sliders.h"
#include "app/ui/hex_color_entry.h"
#include "app/ui/palette_view.h"
#include "app/ui/popup_window_pin.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/tooltips.h"
#include "ui/view.h"

namespace app {
  class PaletteIndexChangeEvent;

  class ColorPopup : public PopupWindowPin
                   , public PaletteViewDelegate {
  public:
    enum SetColorOptions {
      ChangeType,
      DontChangeType
    };

    ColorPopup(const ColorButtonOptions& options);
    ~ColorPopup();

    void setColor(const app::Color& color,
                  const SetColorOptions options);
    app::Color getColor() const;

    // Signals
    obs::signal<void(const app::Color&)> ColorChange;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onWindowResize() override;
    void onMakeFloating() override;
    void onMakeFixed() override;
    void onColorSlidersChange(ColorSlidersChangeEvent& ev);
    void onColorHexEntryChange(const app::Color& color);
    void onSelectOldColor();
    void onSimpleColorClick();
    void onColorTypeClick();
    void onPaletteChange();

    // PaletteViewDelegate impl
    void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) override;

  private:
    void selectColorType(app::Color::Type type);
    void setColorWithSignal(const app::Color& color,
                            const SetColorOptions options);
    void findBestfitIndex(const app::Color& color);
    bool inEditMode();

    class SimpleColors;
    class CustomButtonSet : public ButtonSet {
    public:
      CustomButtonSet();
      int countSelectedItems();
    private:
      void onSelectItem(Item* item, bool focusItem, ui::Message* msg) override;
    };

    ui::Box m_vbox;
    ui::TooltipManager m_tooltips;
    ui::Box m_topBox;
    app::Color m_color;
    Widget* m_closeButton;
    ui::View* m_colorPaletteContainer;
    PaletteView* m_colorPalette;
    SimpleColors* m_simpleColors;
    CustomButtonSet m_colorType;
    HexColorEntry m_hexColorEntry;
    ColorShades m_oldAndNew;
    ColorSliders m_sliders;
    ui::Label m_maskLabel;
    obs::scoped_connection m_onPaletteChangeConn;
    bool m_canPin;
    bool m_insideChange;

    // This variable is used to avoid updating the m_hexColorEntry text
    // when the color change is generated from a
    // HexColorEntry::ColorChange signal. In this way we don't override
    // what the user is writting in the text field.
    bool m_disableHexUpdate;
  };

} // namespace app

#endif
