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

#ifndef TESTS_TEST_H_INCLUDED
#define TESTS_TEST_H_INCLUDED

#include "config.h"

#include <gtest/gtest.h>
#include <allegro.h>

#ifdef TEST_GUI
  #include "gui/gui.h"
#endif

#ifdef main
  #undef main
#endif

int main(int argc, char* argv[])
{
  int exitcode;

  ::testing::InitGoogleTest(&argc, argv);
  allegro_init();

  #ifdef TEST_GUI
    set_color_depth(desktop_color_depth());
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 256, 256, 0, 0);
    install_timer();
    install_keyboard();
    install_mouse();
    {
      Jinete jinete;
      Widget* manager = jmanager_new();
  #endif

      exitcode = RUN_ALL_TESTS();

  #ifdef TEST_GUI
      jmanager_free(manager);
    }
  #endif

  allegro_exit();
  return exitcode;
}

#endif
