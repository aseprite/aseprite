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

#ifdef TEST_GUI
  #include "she/she.h"
  #include "ui/gui.h"
#endif

#ifdef LINKED_WITH_SHE
  #undef main
  #ifdef WIN32
    int main(int argc, char* argv[]) {
      extern int app_main(int argc, char* argv[]);
      return app_main(argc, argv);
    }
  #endif
  #define main app_main
#endif

int main(int argc, char* argv[])
{
  int exitcode;
  ::testing::InitGoogleTest(&argc, argv);

  #ifdef TEST_GUI
    {
      she::ScopedHandle<she::System> system(she::CreateSystem());
      ui::GuiSystem guiSystem;
      UniquePtr<ui::Manager> manager(new ui::Manager());
  #endif

      exitcode = RUN_ALL_TESTS();

  #ifdef TEST_GUI
    }
  #endif

  return exitcode;
}

#endif
