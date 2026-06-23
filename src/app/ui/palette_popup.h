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

#include <string>

#ifdef ENABLE_LOSPEC
  #include "app/lospec.h"
  #include "app/ui/lospec_listbox.h"
  #include "ui/timer.h"
#endif

namespace ui {
class Button;
class View;
} // namespace ui

namespace app {

namespace gen {
class PalettePopup;
}

class HttpLoader;

class PalettePopup : public ui::PopupWindow {
public:
  PalettePopup();
  ~PalettePopup();

  void showPopup(ui::Display* display, const gfx::Rect& buttonPos);

protected:
  bool onProcessMessage(ui::Message* msg) override;

  void onPalChange(const doc::Palette* palette);
  void onSearchChange();
  void onRefresh();
  void onLoadPal();
  void onOpenFolder();

#ifdef ENABLE_LOSPEC
  // Lospec online mode.
  void onToggleLospec();
  void setLospecMode(bool online);
  void onLospecSelChange();
  void onImportPal();
  void onImportUsePal();
  void onImportPalette(const lospec::PaletteInfo& info, bool andUse);
  void onTick();
  void updateButtons();
  // Reads the search term + color-count filter from the widgets and
  // starts a new Lospec search.
  void startLospecSearch();
#endif

private:
  gen::PalettePopup* m_popup;
  PalettesListBox m_paletteListBox;

#ifdef ENABLE_LOSPEC
  LospecListBox m_lospecListBox;
  bool m_lospecMode = false;

  // Background download of the selected Lospec palette to save it as a
  // preset.
  ui::Timer m_importTimer;
  HttpLoader* m_importLoader = nullptr;
  std::string m_importTitle;
  // True if the downloaded palette should also be applied to the active
  // sprite after importing it.
  bool m_importAndUse = false;
#endif
};

} // namespace app

#endif
