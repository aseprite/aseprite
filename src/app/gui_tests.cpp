// Aseprite
// Copyright (C) 2024-2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/modules/gui.h"
#include "ui/message_type.h"
#include "ui/mouse_button.h"

using namespace app;
using namespace ui;

// Regression test for keyboard-only shortcuts triggering on mouse-down events.
// Keyboard-only shortcuts (e.g. Shift) must not consume mouse-down events.
TEST(GUIShortcuts, IgnoreKeyboardOnlyShortcutOnMouseDown)
{
  EXPECT_TRUE(should_ignore_keyboard_only_shortcut_on_mousedown(kMouseDownMessage, kButtonNone));
  EXPECT_FALSE(should_ignore_keyboard_only_shortcut_on_mousedown(kMouseDownMessage, kButtonLeft));
  EXPECT_FALSE(should_ignore_keyboard_only_shortcut_on_mousedown(kKeyDownMessage, kButtonNone));
  EXPECT_FALSE(should_ignore_keyboard_only_shortcut_on_mousedown(kMouseUpMessage, kButtonNone));
}
