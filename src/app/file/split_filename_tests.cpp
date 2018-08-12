// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/file/split_filename.h"

#include "base/fs.h"

using namespace app;

TEST(SplitFilename, Common)
{
  std::string left, right;
  int width;

  EXPECT_EQ(1, split_filename("C:\\test\\a1.png", left, right, width));
  EXPECT_EQ("C:\\test\\a", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(1, width);

  EXPECT_EQ(1, split_filename("C:/test/a1.png", left, right, width));
  EXPECT_EQ("C:/test/a", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(1, width);

  EXPECT_EQ(0, split_filename("file00.png", left, right, width));
  EXPECT_EQ("file", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(2, width);

  EXPECT_EQ(32, split_filename("sprite1-0032", left, right, width));
  EXPECT_EQ("sprite1-", left);
  EXPECT_EQ("", right);
  EXPECT_EQ(4, width);
}

TEST(SplitFilename, InvalidEraseInLeftPart_Issue784)
{
  std::string left, right;
  int width;

  EXPECT_EQ(1, split_filename("by \xE3\x81\xA1\xE3\x81\x83\xE3\x81\xBE\\0001.png", left, right, width));
  EXPECT_EQ("by \xE3\x81\xA1\xE3\x81\x83\xE3\x81\xBE\\", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(4, width);
}
