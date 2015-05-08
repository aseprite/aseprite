// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_BAR_H_INCLUDED
#define APP_UI_COLOR_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/palette_popup.h"
#include "app/ui/palette_view.h"
#include "base/signal.h"
#include "base/unique_ptr.h"
#include "doc/context_observer.h"
#include "doc/pixel_format.h"
#include "doc/sort_palette.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/view.h"

namespace app {
  class ColorButton;
  class PalettesLoader;
  class PaletteIndexChangeEvent;

  class ColorBar : public ui::Box
                 , public PaletteViewDelegate
                 , public doc::ContextObserver {
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

    // ContextObserver impl
    void onActiveSiteChange(const doc::Site& site) override;

    // Signals
    Signal1<void, const app::Color&> FgColorChange;
    Signal1<void, const app::Color&> BgColorChange;

  protected:
    void onPaletteButtonClick();
    void onRemapButtonClick();
    void onPaletteIndexChange(PaletteIndexChangeEvent& ev);
    void onFgColorButtonChange(const app::Color& color);
    void onBgColorButtonChange(const app::Color& color);
    void onColorButtonChange(const app::Color& color);
    void onPickSpectrum(const app::Color& color, ui::MouseButtons buttons);
    void onReverseColors();
    void onSortBy(doc::SortPaletteBy channel);
    void onGradient();
    void setAscending(bool ascending);

    // PaletteViewDelegate impl
    void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) override;
    void onPaletteViewRemapColors(const doc::Remap& remap, const doc::Palette* newPalette) override;
    void onPaletteViewChangeSize(int boxsize) override;

  private:
    void destroyRemap();
    void applyRemap(const doc::Remap& remap, const doc::Palette* newPalette, const std::string& actionText);
    void setPalette(const doc::Palette* newPalette, const std::string& actionText);

    class ScrollableView : public ui::View {
    public:
      ScrollableView();
    protected:
      void onPaint(ui::PaintEvent& ev) override;
    };

    ButtonSet m_buttons;
    PalettePopup m_palettePopup;
    ScrollableView m_scrollableView;
    PaletteView m_paletteView;
    ui::Button m_remapButton;
    ColorButton m_fgColor;
    ColorButton m_bgColor;
    bool m_lock;
    doc::Remap* m_remap;
    const doc::Document* m_lastDocument;
    bool m_ascending;
  };

} // namespace app

#endif
