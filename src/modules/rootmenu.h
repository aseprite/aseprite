/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jbase.h"

enum {
  ACCEL_FOR_UNDO,
  ACCEL_FOR_REDO,
  ACCEL_FOR_FILMEDITOR,
  ACCEL_FOR_SCREENSHOT,
  ACCEL_MAX
};

int init_module_rootmenu();
void exit_module_rootmenu();

JWidget get_root_menu();

JWidget get_recent_list_menuitem();
JWidget get_layer_popup_menu();
JWidget get_frame_popup_menu();
JWidget get_cel_popup_menu();
JWidget get_cel_movement_popup_menu();

/* void show_fx_popup_menu(); */

#endif
