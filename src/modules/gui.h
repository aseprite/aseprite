/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef MODULES_GUI_H_INCLUDED
#define MODULES_GUI_H_INCLUDED

#include "base/exception.h"
#include "skin/skin_property.h"
#include "ui/accelerator.h"
#include "ui/base.h"
#include <list>
#include <string>

class Command;
class Document;
class Params;

namespace ui {
  class ButtonBase;
  class CheckBox;
  class RadioButton;
  class Widget;
  class Window;
  union Message;
}

namespace tools { class Tool; }

int init_module_gui();
void exit_module_gui();

int get_screen_scaling();
void set_screen_scaling(int scaling);

void update_screen_for_document(const Document* document);

void gui_run();
void gui_feedback();
void gui_setup_screen(bool reload_font);

void load_window_pos(ui::Widget* window, const char *section);
void save_window_pos(ui::Widget* window, const char *section);

void setup_mini_look(ui::Widget* widget);
void setup_look(ui::Widget* widget, LookType lookType);
void setup_bevels(ui::Widget* widget, int b1, int b2, int b3, int b4);

void set_gfxicon_to_button(ui::ButtonBase* button,
                           int normal_part_id,
                           int selected_part_id,
                           int disabled_part_id, int icon_align);

ui::CheckBox* check_button_new(const char* text, int b1, int b2, int b3, int b4);

//////////////////////////////////////////////////////////////////////
// Keyboard shortcuts

ui::Accelerator* add_keyboard_shortcut_to_execute_command(const char* shortcut, const char* command_name, Params* params);
ui::Accelerator* add_keyboard_shortcut_to_change_tool(const char* shortcut, tools::Tool* tool);
ui::Accelerator* add_keyboard_shortcut_to_quicktool(const char* shortcut, tools::Tool* tool);
ui::Accelerator* add_keyboard_shortcut_to_spriteeditor(const char* shortcut, const char* action_name);

bool get_command_from_key_message(ui::Message* msg, Command** command, Params** params);
ui::Accelerator* get_accel_to_execute_command(const char* command, Params* params = NULL);
ui::Accelerator* get_accel_to_change_tool(tools::Tool* tool);
ui::Accelerator* get_accel_to_copy_selection();
ui::Accelerator* get_accel_to_snap_to_grid();
ui::Accelerator* get_accel_to_angle_snap();
ui::Accelerator* get_accel_to_maintain_aspect_ratio();
ui::Accelerator* get_accel_to_lock_axis();

tools::Tool* get_selected_quicktool(tools::Tool* currentTool);

#endif
