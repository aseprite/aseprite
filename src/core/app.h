/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef CORE_APP_H
#define CORE_APP_H

#include "jinete/base.h"

int app_init (int argc, char *argv[]);
void app_loop (void);
void app_exit (void);

void app_refresh_screen (void);

void app_realloc_sprite_list (void);
void app_realloc_recent_list (void);

int app_get_current_image_type (void);

JWidget app_get_top_window (void);
JWidget app_get_menu_bar (void);
JWidget app_get_status_bar (void);
JWidget app_get_color_bar (void);
JWidget app_get_tool_bar (void);

void app_switch (JWidget widget);

void app_default_status_bar_message (void);

#endif /* CORE_APP_H */

