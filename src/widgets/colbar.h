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

#ifndef WIDGETS_COLBAR_H
#define WIDGETS_COLBAR_H

#include "jinete/base.h"

#define COLOR_BAR_COLORS   256

typedef struct ColorBar
{
  JWidget widget;
  int ncolor;
  char *color[COLOR_BAR_COLORS];
  int select[2];
} ColorBar;

JWidget color_bar_new (int align);
int color_bar_type (void);

ColorBar *color_bar_data (JWidget color_bar);

void color_bar_set_size (JWidget color_bar, int size);

const char *color_bar_get_color (JWidget color_bar, int num);
void color_bar_set_color (JWidget color_bar, int num, const char *color, bool find);
void color_bar_set_color_directly (JWidget color_bar, int index, const char *color);

#endif /* WIDGETS_COLBAR_H */
