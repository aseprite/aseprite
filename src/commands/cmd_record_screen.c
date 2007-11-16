/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/gfx.h>

#include "jinete/base.h"
#include "jinete/alert.h"

#include "util/recscr.h"

#endif

bool command_checked_record_screen(const char *argument)
{
  return is_rec_screen();
}

void command_execute_record_screen(const char *argument)
{
  if (is_rec_screen())
    rec_screen_off();
  else if (bitmap_color_depth(screen) == 8
	   || jalert(_("Warning"
		       "<<The display isn't in a 8 bpp resolution, the recording."
		       "<<process can be really slow. It's recommended to use a"
		       "<<8 bpp to make it faster."
		       "<<Do you want to continue anyway?"
		       "||Yes||No")) == 1) {
    rec_screen_on();
  }
}
