// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "app/ui/keyboard_shortcuts.h"

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/commands/command.h"
#include "app/commands/command_ids.h"
#include "os/system.h"
#include "ui/app_state.h"
#include "ui/message.h"

using namespace app;
using namespace ui;

static KeyboardShortcuts* ks = nullptr;

#define DEFINE_KEY(commandId, scancode, keycontext)                                                \
  {                                                                                                \
    KeyPtr k = ks->command(CommandId::commandId(), {}, keycontext);                                \
    k->add(ui::Shortcut(kKeyNoneModifier, scancode, 0), KeySource::Original, *ks);                 \
  }

#define DEFINE_USER_KEY(commandId, scancode, keycontext)                                           \
  {                                                                                                \
    KeyPtr k = ks->command(CommandId::commandId(), {}, keycontext);                                \
    k->add(ui::Shortcut(kKeyNoneModifier, scancode, 0), KeySource::UserDefined, *ks);              \
  }

#define EXPECT_COMMAND_FOR_KEY(commandId, scancode, keycontext)                                    \
  {                                                                                                \
    KeyMessage msg(kKeyDownMessage, scancode, kKeyNoneModifier, 0, 0);                             \
    Command* cmd = nullptr;                                                                        \
    Params params;                                                                                 \
    EXPECT_TRUE(ks->getCommandFromKeyMessage(&msg, &cmd, &params, keycontext));                    \
    ASSERT_TRUE(cmd != nullptr) << "command not found for key";                                    \
    EXPECT_EQ(CommandId::commandId(), cmd->id()) << "other command found: " << cmd->id();          \
  }

TEST(KeyboardShortcuts, Basic)
{
  ks->clear();

  KeyPtr k = ks->command(CommandId::Undo(), {}, KeyContext::Any);

  // Simulate a key press and check that the command is not yet
  // associated to the 'Z' key.
  {
    KeyMessage msg(kKeyDownMessage, kKeyZ, kKeyNoneModifier, 0, 0);
    Command* cmd = nullptr;
    Params params;
    EXPECT_FALSE(ks->getCommandFromKeyMessage(&msg, &cmd, &params));
  }

  // Associate the command to the 'Z' key and check.
  k->add(ui::Shortcut(kKeyNoneModifier, kKeyZ, 0), KeySource::Original, *ks);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyZ, KeyContext::Normal);
}

TEST(KeyboardShortcuts, KeyContexts)
{
  ks->clear();

  DEFINE_KEY(Cancel, kKeyEsc, KeyContext::Any);
  DEFINE_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  DEFINE_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);

  EXPECT_COMMAND_FOR_KEY(Cancel, kKeyEsc, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
}

TEST(KeyboardShortcuts, UserDefined)
{
  ks->clear();

  DEFINE_KEY(Undo, kKeyZ, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyZ, KeyContext::Normal);

  DEFINE_USER_KEY(Redo, kKeyZ, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Redo, kKeyZ, KeyContext::Normal);
}

int app_main(int argc, char* argv[])
{
  os::SystemRef system = os::System::make();
  const char* argv2[] = { argv[0] };
  app::AppOptions options(sizeof(argv2) / sizeof(argv2[0]), argv2);
  app::App app;
  app.initialize(options);
  app.run(false);

  ks = KeyboardShortcuts::instance();

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
