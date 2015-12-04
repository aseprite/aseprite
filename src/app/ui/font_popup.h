// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_FONT_POPUP_H_INCLUDED
#define APP_UI_FONT_POPUP_H_INCLUDED
#pragma once

#include "ui/listbox.h"
#include "ui/popup_window.h"

namespace ui {
  class Button;
  class View;
}

namespace app {

  namespace gen {
    class FontPopup;
  }

  class FontPopup : public ui::PopupWindow {
  public:
    FontPopup();

    void showPopup(const gfx::Rect& bounds);

    base::Signal1<void, const std::string&> Load;

  protected:
    void onChangeFont();
    void onLoadFont();

  private:
    gen::FontPopup* m_popup;
    ui::ListBox m_listBox;
  };

} // namespace app

#endif
