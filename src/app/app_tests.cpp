// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/ui/main_window.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/display.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/window.h"

using namespace app;

const char* g_exeName = nullptr;

TEST(App, JustClose)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);
  app.close();
}

TEST(App, ExitCommand)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  ui::execute_from_ui_thread([&app] {
    app.context()->executeCommand(
      Commands::instance()->byId(CommandId::Exit()));
  });

  app.run();
}

TEST(App, ExitWithOneDoc)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  ui::execute_from_ui_thread([&app] {
    Params params;
    params.set("ui", "false");
    app.context()->executeCommand(
      Commands::instance()->byId(CommandId::NewFile()), params);
    app.context()->executeCommand(
      Commands::instance()->byId(CommandId::Exit()));
  });

  app.run();
}

int app_main(int argc, char* argv[])
{
  g_exeName = argv[0];
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
