/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#ifndef MODULES_SPRITES_H
#define MODULES_SPRITES_H

#include "jinete/jbase.h"

struct Image;
struct Layer;
struct Cel;
struct Sprite;

typedef struct ImageRef
{
  struct Image *image;
  struct Layer *layer;
  struct Cel *cel;
  struct ImageRef *next;
} ImageRef;

extern struct Sprite *current_sprite;

int init_module_sprites(void);
void exit_module_sprites(void);

JList get_sprite_list(void);
struct Sprite *get_first_sprite(void);
struct Sprite *get_next_sprite(struct Sprite *sprite);

struct Sprite *get_clipboard_sprite(void);
void set_clipboard_sprite(struct Sprite *sprite);

void sprite_mount(struct Sprite *sprite);
void sprite_unmount(struct Sprite *sprite);

void set_current_sprite(struct Sprite *sprite);
void send_sprite_to_top(struct Sprite *sprite);
void sprite_show(struct Sprite *sprite);

bool is_current_sprite_not_locked(void);
bool is_current_sprite_writable(void);

struct Sprite *lock_current_sprite(void);

ImageRef *sprite_get_images(struct Sprite *sprite, int target, bool write);

#endif /* MODULES_SPRITES_H */

