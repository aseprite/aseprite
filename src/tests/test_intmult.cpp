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

#include "tests/test.h"

#include <errno.h>

#include "raster/blend.h"
#include "raster/image.h"

#ifdef USE_386_ASM
static void test()
{
  register int t;
  int x, y;

  for (x=0; x<256; ++x)
    for (y=0; y<256; ++y)
      if (_int_mult(x, y) != INT_MULT(x, y, t)) {
	assert(false);
      }
}
#endif

int main(int argc, char *argv[])
{
  test_init();

#ifdef USE_386_ASM
  test();
#else
  trace("WARNING: you have to compile with USE_386_ASM\n");
#endif

  return test_exit();
}

END_OF_MAIN();
