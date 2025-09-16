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
    EXPECT_TRUE(ks->getCommandFromKeyMessage(&msg, &cmd, nullptr, keycontext));                    \
    ASSERT_TRUE(cmd != nullptr) << "command not found for key";                                    \
    EXPECT_EQ(CommandId::commandId(), cmd->id()) << "other command found: " << cmd->id();          \
  }

#define NO_COMMAND_FOR_KEY(scancode, keycontext)                                                   \
  {                                                                                                \
    KeyMessage msg(kKeyDownMessage, scancode, kKeyNoneModifier, 0, 0);                             \
    Command* cmd = nullptr;                                                                        \
    EXPECT_FALSE(ks->getCommandFromKeyMessage(&msg, &cmd, nullptr, keycontext));                   \
    ASSERT_TRUE(cmd == nullptr) << "command found for key: " << cmd->id();                         \
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
  DEFINE_KEY(PlayAnimation, kKeyEnter, KeyContext::Normal);
  DEFINE_KEY(Clear, kKeyBackspace, KeyContext::Normal);
  DEFINE_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
  DEFINE_KEY(Apply, kKeyEnter, KeyContext::Transformation);
  DEFINE_KEY(Cut, kKeyX, KeyContext::SelectionTool);

  EXPECT_COMMAND_FOR_KEY(Cancel, kKeyEsc, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(PlayAnimation, kKeyEnter, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(PlayAnimation, kKeyEnter, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(Apply, kKeyEnter, KeyContext::Transformation);
  EXPECT_COMMAND_FOR_KEY(Clear, kKeyBackspace, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Clear, kKeyBackspace, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(Clear, kKeyBackspace, KeyContext::Transformation);

  NO_COMMAND_FOR_KEY(kKeyX, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Cut, kKeyX, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(Cut, kKeyX, KeyContext::Transformation);
}

TEST(KeyboardShortcuts, UserDefinedPriority)
{
  ks->clear();

  DEFINE_KEY(Undo, kKeyZ, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyZ, KeyContext::Normal);

  DEFINE_USER_KEY(Redo, kKeyZ, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Redo, kKeyZ, KeyContext::Normal);

  DEFINE_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
  DEFINE_USER_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);

  DEFINE_USER_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Any);
  EXPECT_COMMAND_FOR_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::SelectionTool);
}

TEST(KeyboardShortcuts, SpecificContextHasMorePriorityButNotIfItsUserDefined)
{
  ks->clear();
  DEFINE_KEY(Cancel, kKeyEsc, KeyContext::Any);
  DEFINE_KEY(Undo, kKeyEsc, KeyContext::Transformation);

  // Pressing "Esc" in "Transformation" context should run "Undo",
  // although "Cancel" is defined for "Any" context, a more specific
  // context should have more priority.
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyEsc, KeyContext::Transformation);

  // But an user-defined key, even for Any context, will overwrite the
  // app-defined shortcut in all contexts.
  DEFINE_USER_KEY(Zoom, kKeyEsc, KeyContext::Any);
  EXPECT_COMMAND_FOR_KEY(Zoom, kKeyEsc, KeyContext::Transformation);
}

// Test that we can configure the Left key to always Undo when the
// default configuration says that the Left key does other actions in
// different contexts.
//
// Related issue: https://github.com/aseprite/aseprite/issues/5390
TEST(KeyboardShortcuts, UndoWithLeftAndRight)
{
  ks->clear();

  DEFINE_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  DEFINE_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(GotoPreviousFrame, kKeyLeft, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(MoveMask, kKeyLeft, KeyContext::SelectionTool);
  // "Transformation" is a sub-context of "Selection" context
  EXPECT_COMMAND_FOR_KEY(MoveMask, kKeyLeft, KeyContext::Transformation);

  // Now we try defining the "Left" key for "Any" context overwriting it in all contexts.
  DEFINE_USER_KEY(Undo, kKeyLeft, KeyContext::Any);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyLeft, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyLeft, KeyContext::SelectionTool);
  EXPECT_COMMAND_FOR_KEY(Undo, kKeyLeft, KeyContext::Transformation);
}

TEST(KeyboardShortcuts, FramesSelection)
{
  ks->clear();

  DEFINE_KEY(LayerProperties, kKeyF2, KeyContext::Normal);
  DEFINE_KEY(SetLoopSection, kKeyF2, KeyContext::FramesSelection);
  EXPECT_COMMAND_FOR_KEY(LayerProperties, kKeyF2, KeyContext::Normal);
  EXPECT_COMMAND_FOR_KEY(SetLoopSection, kKeyF2, KeyContext::FramesSelection);
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
