/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <allegro.h>

#include "core/app.h"

// Information for "ident".

const char ase_ident[] =
    "$ASE: " VERSION " " COPYRIGHT " $\n"
    "$Website: " WEBSITE " $\n";

/***********************************************************************
			     Main Routine
 ***********************************************************************/

int main(int argc, char *argv[])
{
  allegro_init();
  set_uformat(U_ASCII);

#if defined MEMLEAK
  jmemleak_init();
#endif

  // initialises the application
  if (!app_init(argc, argv))
    return 1;

  app_loop();
  app_exit();

#if defined  MEMLEAK
  jmemleak_exit();
#endif

  allegro_exit();
  return 0;
}

END_OF_MAIN();
