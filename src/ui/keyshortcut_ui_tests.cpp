// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#define TEST_GUI
#include "tests/app_test.h"

using namespace ui;

TEST(KeyShortcut, Parser)
{
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyF1, '\0'), KeyShortcut("F1"));
  EXPECT_EQ(KeyShortcut(kKeyAltModifier, kKeyQ, 'q'), KeyShortcut("Alt+Q"));
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyQ, 'q'), KeyShortcut("Ctrl+Q"));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinus, '-'), KeyShortcut("-"));
  EXPECT_EQ(KeyShortcut(kKeyShiftModifier, kKeyMinus, '-'), KeyShortcut("Shift+-"));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyEquals, '='), KeyShortcut("="));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyNil, '+'), KeyShortcut("+"));
  EXPECT_EQ(KeyShortcut(kKeyShiftModifier, kKeyNil, '+'), KeyShortcut("Shift++"));
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyNil, '+'), KeyShortcut("Ctrl++"));

  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinusPad, 0), KeyShortcut("Minus Pad"));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinusPad, 0), KeyShortcut("- Pad"));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyPlusPad, 0), KeyShortcut("Plus Pad"));
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyPlusPad, 0), KeyShortcut("+ Pad"));
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyPlusPad, 0), KeyShortcut("Ctrl++ Pad"));
}

TEST(KeyShortcut, ToString)
{
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyF1, '\0').toString(), KeyShortcut("F1").toString());
  EXPECT_EQ(KeyShortcut(kKeyAltModifier, kKeyQ, 'q').toString(), KeyShortcut("Alt+Q").toString());
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyQ, 'q').toString(), KeyShortcut("Ctrl+Q").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinus, '-').toString(), KeyShortcut("-").toString());
  EXPECT_EQ(KeyShortcut(kKeyShiftModifier, kKeyMinus, '-').toString(), KeyShortcut("Shift+-").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyEquals, '=').toString(), KeyShortcut("=").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyNil, '+').toString(), KeyShortcut("+").toString());
  EXPECT_EQ(KeyShortcut(kKeyShiftModifier, kKeyNil, '+').toString(), KeyShortcut("Shift++").toString());
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyNil, '+').toString(), KeyShortcut("Ctrl++").toString());

  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString(), KeyShortcut("Minus Pad").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString(), KeyShortcut("- Pad").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString(), KeyShortcut("Plus Pad").toString());
  EXPECT_EQ(KeyShortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString(), KeyShortcut("+ Pad").toString());
  EXPECT_EQ(KeyShortcut(kKeyCtrlModifier, kKeyPlusPad, 0).toString(), KeyShortcut("Ctrl++ Pad").toString());

  EXPECT_EQ("- Pad", KeyShortcut(kKeyNoneModifier, kKeyMinusPad, 0).toString());
  EXPECT_EQ("- Pad", KeyShortcut("Minus Pad").toString());
  EXPECT_EQ("- Pad", KeyShortcut("- Pad").toString());

  EXPECT_EQ("+ Pad", KeyShortcut(kKeyNoneModifier, kKeyPlusPad, 0).toString());
  EXPECT_EQ("+ Pad", KeyShortcut("Plus Pad").toString());
  EXPECT_EQ("+ Pad", KeyShortcut("+ Pad").toString());
}
