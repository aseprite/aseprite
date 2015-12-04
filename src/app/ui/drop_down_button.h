// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_DROP_DOWN_BUTTON_H_INCLUDED
#define APP_UI_DROP_DOWN_BUTTON_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "ui/box.h"

namespace ui {
  class Button;
  class Event;
}

namespace app {

  class DropDownButton : public ui::HBox {
  public:
    DropDownButton(const char* text);

    ui::Button* mainButton() { return m_button; }
    ui::Button* dropDown() { return m_dropDown; }

    base::Signal0<void> Click;
    base::Signal0<void> DropDownClick;

  protected:
    void onButtonClick(ui::Event& ev);
    void onDropDownButtonClick(ui::Event& ev);

  private:
    ui::Button* m_button;
    ui::Button* m_dropDown;
  };

} // namespace app

#endif
