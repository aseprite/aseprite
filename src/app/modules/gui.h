/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

  int get_screen_scaling();
  void set_screen_scaling(int scaling);

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
