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

#ifndef WIDGETS_COLVIEW_H_INCLUDED
#define WIDGETS_COLVIEW_H_INCLUDED

#include "jinete/jbase.h"

#include "core/color.h"

// TODO use some JI_SIGNAL_USER
#define SIGNAL_COLORVIEWER_SELECT   0x10002

JWidget colorviewer_new(color_t color, int imgtype);
int colorviewer_type();

int colorviewer_get_imgtype(JWidget colorviewer);

color_t colorviewer_get_color(JWidget colorviewer);
void colorviewer_set_color(JWidget colorviewer, color_t color);

#endif
