// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_MODULES_GUI_H_INCLUDED
#define APP_MODULES_GUI_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_property.h"
#include "base/exception.h"
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

  void update_screen_for_document(Document* document);

  void gui_run();
  void gui_feedback();
  void gui_setup_screen(bool reload_font);

  void load_window_pos(ui::Widget* window, const char *section);
  void save_window_pos(ui::Widget* window, const char *section);

  ui::Widget* setup_mini_font(ui::Widget* widget);
  ui::Widget* setup_mini_look(ui::Widget* widget);
  ui::Widget* setup_look(ui::Widget* widget, skin::LookType lookType);
  void setup_bevels(ui::Widget* widget, int b1, int b2, int b3, int b4);

  void set_gfxicon_to_button(ui::ButtonBase* button,
                             int normal_part_id,
                             int selected_part_id,
                             int disabled_part_id, int icon_align);

  ui::CheckBox* check_button_new(const char* text, int b1, int b2, int b3, int b4);

} // namespace app

#endif
