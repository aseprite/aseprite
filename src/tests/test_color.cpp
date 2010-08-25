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

#include "tests/test.h"

#include "app/color.h"

int main(int argc, char *argv[])
{
  test_init();

  ASSERT(Color::fromRgb(32, 16, 255).getRed() == 32);
  ASSERT(Color::fromRgb(32, 16, 255).getGreen() == 16);
  ASSERT(Color::fromRgb(32, 16, 255).getBlue() == 255);

  ASSERT(Color::fromString("rgb{0,0,0}") == Color::fromRgb(0, 0, 0));
  ASSERT(Color::fromString("rgb{32,16,255}") == Color::fromRgb(32, 16, 255));
  ASSERT(Color::fromString("hsv{32,64,99}") == Color::fromHsv(32, 64, 99));
  ASSERT("rgb{0,0,0}" == Color::fromRgb(0, 0, 0).toString());
  ASSERT("rgb{32,16,255}" == Color::fromRgb(32, 16, 255).toString());
  ASSERT("hsv{32,64,99}" == Color::fromHsv(32, 64, 99).toString());

  return test_exit();
}

END_OF_MAIN();
