/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef MODULES_ROOTMENU_H
#define MODULES_ROOTMENU_H

#include "jinete/jbase.h"

enum {
  ACCEL_FOR_UNDO,
  ACCEL_FOR_REDO,
  ACCEL_FOR_FILMEDITOR,
  ACCEL_FOR_SCREENSHOT,
  ACCEL_MAX
};

int init_module_rootmenu(void);
void exit_module_rootmenu(void);

int load_root_menu(void);

JWidget get_root_menu(void);

JWidget get_recent_list_menuitem(void);
JWidget get_layer_popup_menuitem(void);
JWidget get_frame_popup_menuitem(void);
JWidget get_cel_popup_menuitem(void);

/* int check_for_accel(int accel_type, JMessage msg); */
void show_fx_popup_menu(void);

#endif /* MODULES_ROOTMENU_H */
