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

#ifdef __ASE_CONFIG_H
#  error You cannot use config.h two times
#endif

#define __ASE_CONFIG_H

/* general information */
#define PACKAGE			"ase"
#define VERSION			"0.5"
#define WEBSITE			"http://ase.sourceforge.net/"
#define BUGREPORT		"ase-help@lists.sourceforge.net"
#define COPYRIGHT		"Copyright (C) 2001-2005, 2007 David A. Capello"

#define PRINTF			verbose_printf

/* include everything headers files to precompile */
#ifdef USE_PRECOMPILED_HEADER
#include "all.h"
#endif

/* defined in src/core/core.c */
void verbose_printf(const char *format, ...);

/* strings */
const char *msgids_get(const char *id); /* src/intl/msgids.[ch] */

#define _(msgid) (msgids_get(msgid))

#include <math.h>
#undef PI
#define PI 3.14159265358979323846
