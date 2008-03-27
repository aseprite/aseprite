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

#ifndef CORE_H
#define CORE_H

#include "jinete/jbase.h"

enum {
  MODE_VERBOSE		= 1,	/* verbose mode status */
  MODE_BATCH		= 2,	/* batch mode */
  MODE_CONSOLE		= 4,	/* console mode */
  MODE_GUI		= 8,	/* default GUI mode */
};

extern int ase_mode;

bool core_init(void);
void core_exit(void);

void verbose_printf(const char *format, ...);
bool is_interactive(void);

char *get_pretty_memsize(unsigned int memsize, char *buf, unsigned int bufsize);
char *get_errno_string(int errnum, char *buf, int size);

#endif /* CORE_H */
