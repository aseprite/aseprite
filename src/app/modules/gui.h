// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODULES_GUI_H_INCLUDED
#define APP_MODULES_GUI_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_property.h"
#include "base/exception.h"
#include "gfx/rect.h"
#include "ui/base.h"

namespace ui {
  class ButtonBase;
  class CheckBox;
  class Message;
  class RadioButton;
  class Widget;
  class Window;
}

namespace app {
  class Command;
  class Document;
  class Params;

  namespace tools {
    class Tool;
  }

  int init_module_gui();
  void exit_module_gui();

  void update_screen_for_document(const Document* document);

  void load_window_pos(ui::Widget* window, const char *section);
  void save_window_pos(ui::Widget* window, const char *section);

  ui::Widget* setup_mini_font(ui::Widget* widget);
  ui::Widget* setup_mini_look(ui::Widget* widget);
  ui::Widget* setup_look(ui::Widget* widget, skin::LookType lookType);
  void setup_bevels(ui::Widget* widget, int b1, int b2, int b3, int b4);

  ui::CheckBox* check_button_new(const char* text, int b1, int b2, int b3, int b4);

  // This function can be used to reinvalidate a specific rectangle if
  // we weren't able to validate it on a onPaint() event. E.g. Because
  // the current document was locked.
  void defer_invalid_rect(const gfx::Rect& rc);

} // namespace app

#endif
