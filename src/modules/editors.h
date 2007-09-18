/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#ifndef MODULES_EDITORS_H
#define MODULES_EDITORS_H

#include "jinete/base.h"

extern JWidget current_editor;
extern JWidget box_editors;

struct Sprite;

int init_module_editors(void);
void exit_module_editors(void);

JWidget create_new_editor(void);
void remove_editor(JWidget editor);

void set_current_editor(JWidget editor);

void refresh_all_editors(void);
void update_editors_with_sprite(struct Sprite *sprite);
void editors_draw_sprite(struct Sprite *sprite, int x1, int y1, int x2, int y2);
void editors_draw_sprite_tiled(struct Sprite *sprite, int x1, int y1, int x2, int y2);
void editors_hide_sprite(struct Sprite *sprite);
void replace_sprite_in_editors(struct Sprite *old_sprite, struct Sprite *new_sprite);

void set_sprite_in_current_editor(struct Sprite *sprite);
void set_sprite_in_more_reliable_editor(struct Sprite *sprite);

void split_editor(JWidget editor, int align);
void close_editor(JWidget editor);
void make_unique_editor(JWidget editor);

#endif /* MODULES_EDITORS_H */

