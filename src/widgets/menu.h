/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#ifndef WIDGETS_MENU_H
#define WIDGETS_MENU_H

#include "jinete/base.h"
#include "modules/chkmthds.h"

JWidget menuitem_new(const char *text);

const char *menuitem_get_script(JWidget menuitem);
void menuitem_set_script(JWidget menuitem, const char *format, ...);

void menuitem_set_check_method(JWidget menuitem, CheckMethod check_method);

#endif /* WIDGETS_MENU_H */
