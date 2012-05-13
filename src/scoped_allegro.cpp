/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "scoped_allegro.h"

#include <allegro.h>
#include "loadpng.h"

ScopedAllegro::ScopedAllegro()
{
  allegro_init();
  set_uformat(U_ASCII);
  install_timer();

  // Register PNG as a supported bitmap type
  register_bitmap_file_type("png", load_png, save_png);
}

ScopedAllegro::~ScopedAllegro()
{
  remove_timer();
  allegro_exit();
}
