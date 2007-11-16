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

#ifndef WIDGETS_COLVIEW_H
#define WIDGETS_COLVIEW_H

#include "jinete/base.h"

/* XXX use some JI_SIGNAL_USER */
#define SIGNAL_COLOR_VIEWER_SELECT   0x10002

JWidget color_viewer_new (const char *color, int imgtype);
int color_viewer_type (void);

int color_viewer_get_imgtype (JWidget color_viewer_id);

const char *color_viewer_get_color (JWidget color_viewer);
void color_viewer_set_color (JWidget color_viewer, const char *color);

#endif /* WIDGETS_COLVIEW_H */
