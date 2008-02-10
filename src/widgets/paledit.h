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

#ifndef WIDGETS_PALEDIT_H
#define WIDGETS_PALEDIT_H

#include <allegro/color.h>

#include "jinete/jbase.h"

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_PALETTE_EDITOR_CHANGE   0x10005

enum {
  PALETTE_EDITOR_RANGE_NONE,
  PALETTE_EDITOR_RANGE_LINEAL,
  PALETTE_EDITOR_RANGE_RECTANGULAR,
};

typedef struct PaletteEditor
{
  JWidget widget;
  RGB *palette;
  unsigned editable : 8;
  unsigned range_type : 8;
  unsigned columns : 16;
  int boxsize;
  int color[2];
} PaletteEditor;

JWidget palette_editor_new (RGB *palette, int editable, int boxsize);
int palette_editor_type (void);

PaletteEditor *palette_editor_data (JWidget palette_editor);

void palette_editor_set_columns (JWidget palette_editor, int columns);

void palette_editor_select_color (JWidget palette_editor, int index);
void palette_editor_select_range (JWidget palette_editor, int begin, int end, int range_type);

void palette_editor_move_selection (JWidget palette_editor, int x, int y);

void palette_editor_get_selected_entries(JWidget widget, bool array[256]);

#endif /* WIDGETS_PALEDIT_H */
