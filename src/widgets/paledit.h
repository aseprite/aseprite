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

#ifndef WIDGETS_PALEDIT_H
#define WIDGETS_PALEDIT_H

#include <allegro/color.h>

#include "jinete/jbase.h"

struct Palette;

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_PALETTE_EDITOR_CHANGE   0x10005

enum {
  PALETTE_EDITOR_RANGE_NONE,
  PALETTE_EDITOR_RANGE_LINEAL,
  PALETTE_EDITOR_RANGE_RECTANGULAR,
};

JWidget paledit_new(struct Palette *palette, bool editable, int boxsize);
int paledit_type();

struct Palette *paledit_get_palette(JWidget widget);
int paledit_get_range_type(JWidget widget);

int paledit_get_columns(JWidget widget);
void paledit_set_columns(JWidget widget, int columns);
void paledit_set_boxsize(JWidget widget, int boxsize);

void paledit_select_color(JWidget widget, int index);
void paledit_select_range(JWidget widget, int begin, int end, int range_type);

void paledit_move_selection(JWidget widget, int x, int y);

int paledit_get_1st_color(JWidget widget);
int paledit_get_2nd_color(JWidget widget);
void paledit_get_selected_entries(JWidget widget, bool array[256]);

#endif /* WIDGETS_PALEDIT_H */
