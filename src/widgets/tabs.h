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

#ifndef WIDGETS_TABS_H
#define WIDGETS_TABS_H

#include "jinete/jbase.h"

JWidget tabs_new(void (*select_callback)(JWidget tabs, void *data));

void tabs_append_tab(JWidget widget, const char *text, void *data);
void tabs_remove_tab(JWidget widget, void *data);

void tabs_set_text_for_tab(JWidget widget, const char *text, void *data);

void tabs_select_tab(JWidget widget, void *data);
void *tabs_get_selected_tab(JWidget widget);

#endif /* WIDGETS_TABS_H */
