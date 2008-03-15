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

#ifndef WIDGETS_COLBUT_H
#define WIDGETS_COLBUT_H

#include "core/color.h"

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_COLOR_BUTTON_CHANGE   0x10001

JWidget color_button_new(color_t color, int imgtype);
int color_button_type(void);

int color_button_get_imgtype(JWidget color_button);

color_t color_button_get_color(JWidget color_button);
void color_button_set_color(JWidget color_button, color_t color);

#endif /* WIDGETS_COLBUT_H */
