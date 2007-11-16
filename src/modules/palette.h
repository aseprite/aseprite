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

#ifndef MODULES_PALETTE_H
#define MODULES_PALETTE_H

#include "jinete/base.h"
#include <allegro/color.h>

struct Sprite;

extern PALETTE current_palette;
extern RGB_MAP *orig_rgb_map;
extern COLOR_MAP *orig_trans_map;

int init_module_palette (void);
void exit_module_palette (void);

void set_default_palette (struct RGB *palette);
bool set_current_palette (struct RGB *palette, int forced);
void set_current_color (int index, int r, int g, int b);

void hook_palette_changes (void (*proc)(void));
void unhook_palette_changes (void (*proc)(void));
void call_palette_hooks (void);

void use_current_sprite_rgb_map (void);
void use_sprite_rgb_map (struct Sprite *sprite);
void restore_rgb_map (void);

void make_palette_ramp (RGB *palette, int from, int to);
void make_palette_rect_ramp (RGB *palette, int from, int to, int width);

int palette_diff(RGB *p1, RGB *p2, int *from, int *to);
void palette_copy(RGB *dst, RGB *src);
RGB *palette_load(const char *filename);
int palette_save(RGB *palette, const char *filename);

#endif /* MODULES_PALETTE_H */

