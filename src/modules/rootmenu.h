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

#ifndef MODULES_ROOTMENU_H_INCLUDED
#define MODULES_ROOTMENU_H_INCLUDED

#include "gui/base.h"

namespace ui {
  class Menu;
  class MenuItem;
}

enum {
  ACCEL_FOR_UNDO,
  ACCEL_FOR_REDO,
  ACCEL_FOR_FILMEDITOR,
  ACCEL_FOR_SCREENSHOT,
  ACCEL_MAX
};

int init_module_rootmenu();
void exit_module_rootmenu();

ui::Menu* get_root_menu();

ui::MenuItem* get_recent_list_menuitem();
ui::Menu* get_document_tab_popup_menu();
ui::Menu* get_layer_popup_menu();
ui::Menu* get_frame_popup_menu();
ui::Menu* get_cel_popup_menu();
ui::Menu* get_cel_movement_popup_menu();

#endif
