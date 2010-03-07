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

#ifndef MODULES_EDITORS_H_INCLUDED
#define MODULES_EDITORS_H_INCLUDED

#include "jinete/jbase.h"
#include "core/color.h"

class Editor;

extern Editor* current_editor;
extern JWidget box_editors;

class Sprite;

int init_module_editors();
void exit_module_editors();

Editor* create_new_editor();
void remove_editor(Editor* editor);

void set_current_editor(Editor* editor);

void refresh_all_editors();
void update_editors_with_sprite(const Sprite* sprite);
void editors_draw_sprite(const Sprite* sprite, int x1, int y1, int x2, int y2);
void editors_draw_sprite_tiled(const Sprite* sprite, int x1, int y1, int x2, int y2);
void editors_hide_sprite(const Sprite* sprite);

void set_sprite_in_current_editor(Sprite* sprite);
void set_sprite_in_more_reliable_editor(Sprite* sprite);

void split_editor(Editor* editor, int align);
void close_editor(Editor* editor);
void make_unique_editor(Editor* editor);

#endif

