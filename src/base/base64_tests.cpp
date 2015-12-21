// Aseprite Base Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/base64.h"

using namespace base;

TEST(Base64, Encode)
{
  buffer data;
  data.push_back('a');
  data.push_back('b');
  data.push_back('c');
  data.push_back('d');
  data.push_back('e');

  std::string output;
  encode_base64(data, output);
  EXPECT_EQ("YWJjZGU=", output);
}

TEST(Base64, Decode)
{
  buffer output;
  decode_base64("YWJjZGU=", output);

  std::string output_str;
  for (auto chr : output)
    output_str.push_back(chr);

  EXPECT_EQ("abcde", output_str);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
