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

#ifndef WIDGETS_COLBUT_H_INCLUDED
#define WIDGETS_COLBUT_H_INCLUDED

#include "core/color.h"

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_COLORBUTTON_CHANGE   0x10001

JWidget colorbutton_new(color_t color, int imgtype);
int colorbutton_type();

int colorbutton_get_imgtype(JWidget widget);

color_t colorbutton_get_color(JWidget widget);
void colorbutton_set_color(JWidget widget, color_t color);

#endif
