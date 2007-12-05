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

#ifndef CORE_CFG_H
#define CORE_CFG_H

#include <allegro/config.h>
#include "jinete/jbase.h"

void ase_config_init(void);
void ase_config_exit(void);

bool get_config_bool(const char *section, const char *name, bool value);
void set_config_bool(const char *section, const char *name, bool value);

void get_config_rect(const char *section, const char *name, JRect rect);
void set_config_rect(const char *section, const char *name, JRect rect);

#endif /* CORE_CFG_H */
