// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#define TEST_GUI
#include "tests/app_test.h"

using namespace ui;

TEST(Accelerator, Parser)
{
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyF1, '\0'), Accelerator("F1"));
  EXPECT_EQ(Accelerator(kKeyAltModifier, kKeyQ, 'q'), Accelerator("Alt+Q"));
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyQ, 'q'), Accelerator("Ctrl+Q"));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinus, '-'), Accelerator("-"));
  EXPECT_EQ(Accelerator(kKeyShiftModifier, kKeyMinus, '-'), Accelerator("Shift+-"));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyEquals, '='), Accelerator("="));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyNil, '+'), Accelerator("+"));
  EXPECT_EQ(Accelerator(kKeyShiftModifier, kKeyNil, '+'), Accelerator("Shift++"));
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyNil, '+'), Accelerator("Ctrl++"));

  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinusPad, 0), Accelerator("Minus Pad"));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinusPad, 0), Accelerator("- Pad"));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyPlusPad, 0), Accelerator("Plus Pad"));
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyPlusPad, 0), Accelerator("+ Pad"));
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyPlusPad, 0), Accelerator("Ctrl++ Pad"));
}

TEST(Accelerator, ToString)
{
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyF1, '\0').toString(), Accelerator("F1").toString());
  EXPECT_EQ(Accelerator(kKeyAltModifier, kKeyQ, 'q').toString(), Accelerator("Alt+Q").toString());
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyQ, 'q').toString(), Accelerator("Ctrl+Q").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinus, '-').toString(), Accelerator("-").toString());
  EXPECT_EQ(Accelerator(kKeyShiftModifier, kKeyMinus, '-').toString(),
            Accelerator("Shift+-").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyEquals, '=').toString(), Accelerator("=").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyNil, '+').toString(), Accelerator("+").toString());
  EXPECT_EQ(Accelerator(kKeyShiftModifier, kKeyNil, '+').toString(),
            Accelerator("Shift++").toString());
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyNil, '+').toString(),
            Accelerator("Ctrl++").toString());

  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinusPad, 0).toString(),
            Accelerator("Minus Pad").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyMinusPad, 0).toString(),
            Accelerator("- Pad").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyPlusPad, 0).toString(),
            Accelerator("Plus Pad").toString());
  EXPECT_EQ(Accelerator(kKeyNoneModifier, kKeyPlusPad, 0).toString(),
            Accelerator("+ Pad").toString());
  EXPECT_EQ(Accelerator(kKeyCtrlModifier, kKeyPlusPad, 0).toString(),
            Accelerator("Ctrl++ Pad").toString());

  EXPECT_EQ("- Pad", Accelerator(kKeyNoneModifier, kKeyMinusPad, 0).toString());
  EXPECT_EQ("- Pad", Accelerator("Minus Pad").toString());
  EXPECT_EQ("- Pad", Accelerator("- Pad").toString());

  EXPECT_EQ("+ Pad", Accelerator(kKeyNoneModifier, kKeyPlusPad, 0).toString());
  EXPECT_EQ("+ Pad", Accelerator("Plus Pad").toString());
  EXPECT_EQ("+ Pad", Accelerator("+ Pad").toString());
}
