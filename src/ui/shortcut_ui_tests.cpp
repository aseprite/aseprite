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

TEST(Shortcut, Sequences)
{
  // Test basic sequence parsing
  Shortcut seq1("Ctrl+K Ctrl+O");
  EXPECT_TRUE(seq1.isSequence());
  EXPECT_EQ(2, seq1.sequenceSize());
  EXPECT_EQ("Ctrl+K Ctrl+O", seq1.toString());
  
  // Test that single shortcuts are not sequences
  Shortcut single("Ctrl+K");
  EXPECT_FALSE(single.isSequence());
  EXPECT_EQ(1, single.sequenceSize());
  
  // Test sequence creation with MakeSequence
  std::vector<Shortcut> keys;
  keys.push_back(Shortcut("Ctrl+K"));
  keys.push_back(Shortcut("Ctrl+O"));
  Shortcut seq2 = Shortcut::MakeSequence(keys);
  EXPECT_TRUE(seq2.isSequence());
  EXPECT_EQ(2, seq2.sequenceSize());
  EXPECT_EQ("Ctrl+K Ctrl+O", seq2.toString());
  
  // Test getKeyAt
  EXPECT_EQ("Ctrl+K", seq2.getKeyAt(0).toString());
  EXPECT_EQ("Ctrl+O", seq2.getKeyAt(1).toString());
  
  // Test sequence equality
  EXPECT_EQ(seq1, seq2);
  
  // Test 3-key sequence
  Shortcut seq3("Ctrl+K Ctrl+A Ctrl+B");
  EXPECT_TRUE(seq3.isSequence());
  EXPECT_EQ(3, seq3.sequenceSize());
  EXPECT_EQ("Ctrl+K Ctrl+A Ctrl+B", seq3.toString());
  
  // Test that sequences don't interfere with "Space" keyword
  Shortcut spaceKey("Ctrl+Space");
  EXPECT_FALSE(spaceKey.isSequence());
  EXPECT_EQ("Ctrl+Space", spaceKey.toString());
}
