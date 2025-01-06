// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#define TEST_GUI
#include "tests/app_test.h"

using namespace ui;

TEST(Shortcut, Parser)
{
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyF1, '\0'), Shortcut("F1"));
  EXPECT_EQ(Shortcut(kKeyAltModifier, kKeyQ, 'q'), Shortcut("Alt+Q"));
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyQ, 'q'), Shortcut("Ctrl+Q"));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinus, '-'), Shortcut("-"));
  EXPECT_EQ(Shortcut(kKeyShiftModifier, kKeyMinus, '-'), Shortcut("Shift+-"));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyEquals, '='), Shortcut("="));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyNil, '+'), Shortcut("+"));
  EXPECT_EQ(Shortcut(kKeyShiftModifier, kKeyNil, '+'), Shortcut("Shift++"));
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyNil, '+'), Shortcut("Ctrl++"));

  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinusPad, 0), Shortcut("Minus Pad"));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinusPad, 0), Shortcut("- Pad"));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyPlusPad, 0), Shortcut("Plus Pad"));
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyPlusPad, 0), Shortcut("+ Pad"));
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyPlusPad, 0), Shortcut("Ctrl++ Pad"));
}

TEST(Shortcut, ToString)
{
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyF1, '\0').toString(), Shortcut("F1").toString());
  EXPECT_EQ(Shortcut(kKeyAltModifier, kKeyQ, 'q').toString(), Shortcut("Alt+Q").toString());
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyQ, 'q').toString(), Shortcut("Ctrl+Q").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinus, '-').toString(), Shortcut("-").toString());
  EXPECT_EQ(Shortcut(kKeyShiftModifier, kKeyMinus, '-').toString(), Shortcut("Shift+-").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyEquals, '=').toString(), Shortcut("=").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyNil, '+').toString(), Shortcut("+").toString());
  EXPECT_EQ(Shortcut(kKeyShiftModifier, kKeyNil, '+').toString(), Shortcut("Shift++").toString());
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyNil, '+').toString(), Shortcut("Ctrl++").toString());

  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString(),
            Shortcut("Minus Pad").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString(), Shortcut("- Pad").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString(), Shortcut("Plus Pad").toString());
  EXPECT_EQ(Shortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString(), Shortcut("+ Pad").toString());
  EXPECT_EQ(Shortcut(kKeyCtrlModifier, kKeyPlusPad, 0).toString(),
            Shortcut("Ctrl++ Pad").toString());

  EXPECT_EQ("- Pad", Shortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString());
  EXPECT_EQ("- Pad", Shortcut("Minus Pad").toString());
  EXPECT_EQ("- Pad", Shortcut("- Pad").toString());

  EXPECT_EQ("+ Pad", Shortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString());
  EXPECT_EQ("+ Pad", Shortcut("Plus Pad").toString());
  EXPECT_EQ("+ Pad", Shortcut("+ Pad").toString());
}
