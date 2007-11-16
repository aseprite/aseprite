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

#ifndef MODULES_GUI_H
#define MODULES_GUI_H

#include "jinete/base.h"

#define HOOK(widget, signal, signal_handler, user_data)			\
  hook_signal ((widget), (signal), (signal_handler), (int)(user_data))

struct Sprite;

int init_module_gui(void);
void exit_module_gui(void);

void GUI_Refresh(struct Sprite *sprite);

void gui_run(void);
void gui_feedback(void);
void gui_setup_screen(void);

void reload_default_font(void);

void load_window_pos(JWidget window, const char *section);
void save_window_pos(JWidget window, const char *section);

JWidget load_widget(const char *filename, const char *name);

void rebuild_lock(void);
void rebuild_unlock(void);
void rebuild_root_menu(void);
void rebuild_sprite_list(void);
void rebuild_recent_list(void);

void hook_signal(JWidget widget,
		 int signal_num,
		 int(*signal_handler) (JWidget widget, int user_data),
		 int user_data);

bool get_widgets(JWidget window, ...);

void add_gfxicon_to_button(JWidget button, int gfx_id, int icon_align);
void set_gfxicon_in_button(JWidget button, int gfx_id);

JWidget radio_button_new(int radio_group, int b1, int b2, int b3, int b4);
JWidget check_button_new(const char *text, int b1, int b2, int b3, int b4);
/* void change_to_button_style(JWidget widget, int b1, int b2, int b3, int b4); */

#endif /* MODULES_GUI_H */
