// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FONT_POPUP_H_INCLUDED
#define APP_UI_FONT_POPUP_H_INCLUDED
#pragma once

#include "app/task.h"
#include "ui/listbox.h"
#include "ui/popup_window.h"
#include "ui/timer.h"

namespace ui {
class Button;
class View;
} // namespace ui

namespace app {

namespace gen {
class FontPopup;
}

class FontInfo;

class FontPopup : public ui::PopupWindow {
public:
  class FontListBox : public ui::ListBox {
  protected:
    bool onProcessMessage(ui::Message* msg) override;
    bool onAcceptKeyInput() override;
  };

  FontPopup(const FontInfo& fontInfo);
  ~FontPopup();

  void setSearchText(const std::string& searchText);

  void showPopup(ui::Display* display, const gfx::Rect& buttonBounds);

  void recreatePinnedItems();

  FontListBox* getListBox() { return &m_listBox; }
  FontInfo selectedFont();

  obs::signal<void(const FontInfo&)> FontChange;
  obs::signal<void()> EscKey;

protected:
  void onFontChange();
  void onLoadFont();
  void onThumbnailGenerated();
  void onTickRelayout();
  bool onProcessMessage(ui::Message* msg) override;

private:
  void listSystemFonts(base::task_token& token);

  gen::FontPopup* m_popup;
  Widget* m_systemFontsSeparator;
  FontListBox m_listBox;
  ui::Timer m_timer;
  ui::Widget* m_pinnedSeparator = nullptr;
  app::Task m_listFontsTask;
};

} // namespace app

#endif
