// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PALETTE_POPUP_H_INCLUDED
#define APP_UI_PALETTE_POPUP_H_INCLUDED
#pragma once

#include "app/ui/palettes_listbox.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class View;
}

namespace app {

  namespace gen {
    class PalettePopup;
  }

  class PalettePopup : public ui::PopupWindow {
  public:
    PalettePopup();

    void showPopup(ui::Display* display,
                   const gfx::Rect& buttonPos);

  protected:
    bool onProcessMessage(ui::Message* msg) override;

    void onPalChange(const doc::Palette* palette);
    void onSearchChange();
    void onRefresh();
    void onLoadPal();
    void onOpenFolder();

  private:
    gen::PalettePopup* m_popup;
    PalettesListBox m_paletteListBox;
  };

} // namespace app

#endif
