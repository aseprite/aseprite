// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef TESTS_TEST_H_INCLUDED
#define TESTS_TEST_H_INCLUDED
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#ifdef TEST_GUI
  #include "she/she.h"
  #include "ui/ui.h"
#endif

#ifdef LINKED_WITH_SHE
  #undef main
  #ifdef _WIN32
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
      // Do not create a she::System, as we don't need it for testing purposes.
      //she::ScopedHandle<she::System> system(she::create_system());
      ui::UISystem uiSystem;
      ui::Manager uiManager;
  #endif

      exitcode = RUN_ALL_TESTS();

  #ifdef TEST_GUI
    }
  #endif

  return exitcode;
}

#endif
