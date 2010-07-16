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

#include "config.h"

#include <allegro/unicode.h>

char *get_pretty_memory_size(unsigned int memsize, char *buf, unsigned int bufsize)
{
  if (memsize < 1000)
    uszprintf(buf, bufsize, "%d bytes", memsize);
  else if (memsize < 1000*1000)
    uszprintf(buf, bufsize, "%0.1fK", memsize/1024.0f);
  else
    uszprintf(buf, bufsize, "%0.1fM", memsize/(1024.0f*1024.0f));

  return buf;
}
