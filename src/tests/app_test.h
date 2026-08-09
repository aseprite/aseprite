// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
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

#ifdef TEST_APP
  #include "app/app.h"
  #include "app/cli/app_options.h"
#endif

#if defined(TEST_GUI) || defined(TEST_APP)
  #include "os/os.h"
  #include "ui/ui.h"
#endif

#ifdef LINKED_WITH_OS_LIBRARY
  #undef main
  #ifdef _WIN32
int main(int argc, char* argv[])
{
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

#if defined(TEST_GUI) || defined(TEST_APP)
  {
    os::SystemRef system = os::System::make();
    ui::UISystem uiSystem;
    ui::Manager uiManager(nullptr);
    ui::Theme uiTheme;
    ui::set_theme(&uiTheme, 1);
#endif
#ifdef TEST_APP
    auto* exeName = argv[0];
    const char* argv[] = { exeName, "--batch" };
    const app::AppOptions options(std::size(argv), argv);
    app::App app;
    app.initialize(options);
#endif

    exitcode = RUN_ALL_TESTS();

#if defined(TEST_GUI) || defined(TEST_APP)
  }
#endif

  return exitcode;
}

#endif
