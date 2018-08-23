// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef TESTS_TEST_H_INCLUDED
#define TESTS_TEST_H_INCLUDED
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#ifdef TEST_GUI
  #include "os/os.h"
  #include "ui/ui.h"
#endif

#ifdef LINKED_WITH_OS_LIBRARY
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
      // Do not create a os::System, as we don't need it for testing purposes.
      //os::ScopedHandle<os::System> system(os::create_system());
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
