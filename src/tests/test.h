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

#ifndef TESTS_TEST_H_INCLUDED
#define TESTS_TEST_H_INCLUDED

#ifdef NDEBUG
#error You should compile the tests with the NDEBUG flag activated
#endif

#include <stdio.h>
#include <allegro.h>

#include "config.h"

#ifdef TEST_GUI
  #include "jinete/jinete.h"
#endif

void trace(const char *format, ...)
{
  char buf[4096];
  va_list ap;

  va_start(ap, format);
  uvszprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  fputs(buf, stdout);
  fflush(stdout);
}

#ifndef TEST_GUI

void test_init()
{
  allegro_init();
}

int test_exit()
{
  allegro_exit();
  return 0;
}

#else

JWidget test_init_gui()
{
  allegro_init();
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, 256, 256, 0, 0);
  install_timer();
  install_keyboard();
  install_mouse();

  return jmanager_new();
}

int test_exit_gui(JWidget manager)
{
  jmanager_free(manager);
  allegro_exit();
  return 0;
}

#endif

#endif
