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

#ifndef WIDGETS_COLBAR_H
#define WIDGETS_COLBAR_H

#include "jinete/jbase.h"

#include "core/color.h"

JWidget colorbar_new(int align);
int colorbar_type();

void colorbar_set_size(JWidget widget, int size);

color_t colorbar_get_fg_color(JWidget widget);
color_t colorbar_get_bg_color(JWidget widget);

void colorbar_set_fg_color(JWidget widget, color_t color);
void colorbar_set_bg_color(JWidget widget, color_t color);

void colorbar_set_color(JWidget widget, int index, color_t color);

color_t colorbar_get_color_by_position(JWidget widget, int x, int y);

#endif /* WIDGETS_COLBAR_H */
