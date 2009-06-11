/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jbase.h"

/* enumeration of ASE events in the highest application level */
enum {
  APP_EXIT,
  APP_PALETTE_CHANGE,
  APP_EVENTS
};

class Layer;
class Sprite;

bool app_init(int argc, char *argv[]);
void app_loop();
void app_exit();

void app_add_hook(int app_event, void (*proc)(void *data), void *data);
void app_trigger_event(int app_event);

void app_refresh_screen(const Sprite* sprite);

void app_realloc_sprite_list();
bool app_realloc_recent_list();

int app_get_current_image_type();

JWidget app_get_top_window();
JWidget app_get_menubar();
JWidget app_get_statusbar();
JWidget app_get_colorbar();
JWidget app_get_toolbar();
JWidget app_get_tabsbar();

void app_default_statusbar_message();

int app_get_fg_color(Sprite* sprite);
int app_get_bg_color(Sprite* sprite);
int app_get_color_to_clear_layer(Layer* layer);

#endif /* CORE_APP_H */

