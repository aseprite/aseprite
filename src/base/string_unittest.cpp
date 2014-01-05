// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/string.h"

#include <algorithm>

using namespace base;

bool all(int) { return true; }

TEST(String, Utf8Conversion)
{
  std::string a = "\xE6\xBC\xA2\xE5\xAD\x97"; // 漢字
  ASSERT_EQ(6, a.size());

  std::wstring b = from_utf8(a);
  ASSERT_EQ(2, b.size());
  ASSERT_EQ(0x6f22, b[0]);
  ASSERT_EQ(0x5b57, b[1]);

  std::string c = to_utf8(b);
  ASSERT_EQ(a, c);
}

TEST(String, Utf8Iterator)
{
  string a = "Hello";
  int value = std::count_if(utf8_iterator(a.begin()),
                            utf8_iterator(a.end()), all);
  ASSERT_EQ(5, value);
  ASSERT_EQ('H', *(utf8_iterator(a.begin())));
  ASSERT_EQ('e', *(utf8_iterator(a.begin())+1));
  ASSERT_EQ('l', *(utf8_iterator(a.begin())+2));
  ASSERT_EQ('l', *(utf8_iterator(a.begin())+3));
  ASSERT_EQ('o', *(utf8_iterator(a.begin())+4));

  string b = "Copyright \xC2\xA9";
  value = std::count_if(utf8_iterator(b.begin()),
                        utf8_iterator(b.end()), all);
  ASSERT_EQ(11, value);
  ASSERT_EQ('C', *(utf8_iterator(b.begin())));
  ASSERT_EQ('o', *(utf8_iterator(b.begin())+1));
  ASSERT_EQ(0xA9, *(utf8_iterator(b.begin())+10));
  ASSERT_TRUE((utf8_iterator(b.begin())+11) == utf8_iterator(b.end()));

  string c = "\xf0\x90\x8d\x86\xe6\x97\xa5\xd1\x88";
  value = std::count_if(utf8_iterator(c.begin()),
                        utf8_iterator(c.end()), all);
  ASSERT_EQ(3, value);
  ASSERT_EQ(0x10346, *(utf8_iterator(c.begin())));
  ASSERT_EQ(0x65E5, *(utf8_iterator(c.begin())+1));
  ASSERT_EQ(0x448, *(utf8_iterator(c.begin())+2));
  ASSERT_TRUE((utf8_iterator(c.begin())+3) == utf8_iterator(c.end()));

  string d = "\xf0\xa4\xad\xa2";
  value = std::count_if(utf8_iterator(d.begin()),
                        utf8_iterator(d.end()), all);
  ASSERT_EQ(1, value);
  ASSERT_EQ(0x24B62, *(utf8_iterator(d.begin())));
  ASSERT_TRUE((utf8_iterator(d.begin())+1) == utf8_iterator(d.end()));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
