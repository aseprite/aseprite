// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/string.h"

#include <algorithm>
#include <clocale>

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

TEST(String, Utf8Wrapper)
{
  std::string a, b = "abc";
  for (int ch : utf8(b))
    a.push_back(ch);
  EXPECT_EQ("abc", a);

  std::string c, d = "def";
  for (int ch : utf8_const(d))  // TODO we should be able to specify a string-literal here
    c.push_back(ch);
  EXPECT_EQ("def", c);

  int i = 0;
  d = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
  for (int ch : utf8_const(d)) { // 日本語
    switch (i++) {
      case 0: EXPECT_EQ(ch, 0x65E5); break;
      case 1: EXPECT_EQ(ch, 0x672C); break;
      case 2: EXPECT_EQ(ch, 0x8A9E); break;
      default: EXPECT_FALSE(true); break;
    }
  }
}

TEST(String, Utf8Iterator)
{
  std::string a = "Hello";
  int value = std::count_if(utf8_iterator(a.begin()),
                            utf8_iterator(a.end()), all);
  ASSERT_EQ(5, value);
  ASSERT_EQ('H', *(utf8_iterator(a.begin())));
  ASSERT_EQ('e', *(utf8_iterator(a.begin())+1));
  ASSERT_EQ('l', *(utf8_iterator(a.begin())+2));
  ASSERT_EQ('l', *(utf8_iterator(a.begin())+3));
  ASSERT_EQ('o', *(utf8_iterator(a.begin())+4));

  std::string b = "Copyright \xC2\xA9";
  value = std::count_if(utf8_iterator(b.begin()),
                        utf8_iterator(b.end()), all);
  ASSERT_EQ(11, value);
  ASSERT_EQ('C', *(utf8_iterator(b.begin())));
  ASSERT_EQ('o', *(utf8_iterator(b.begin())+1));
  ASSERT_EQ(0xA9, *(utf8_iterator(b.begin())+10));
  ASSERT_TRUE((utf8_iterator(b.begin())+11) == utf8_iterator(b.end()));

  std::string c = "\xf0\x90\x8d\x86\xe6\x97\xa5\xd1\x88";
  value = std::count_if(utf8_iterator(c.begin()),
                        utf8_iterator(c.end()), all);
  ASSERT_EQ(3, value);
  ASSERT_EQ(0x10346, *(utf8_iterator(c.begin())));
  ASSERT_EQ(0x65E5, *(utf8_iterator(c.begin())+1));
  ASSERT_EQ(0x448, *(utf8_iterator(c.begin())+2));
  ASSERT_TRUE((utf8_iterator(c.begin())+3) == utf8_iterator(c.end()));

  std::string d = "\xf0\xa4\xad\xa2";
  value = std::count_if(utf8_iterator(d.begin()),
                        utf8_iterator(d.end()), all);
  ASSERT_EQ(1, value);
  ASSERT_EQ(0x24B62, *(utf8_iterator(d.begin())));
  ASSERT_TRUE((utf8_iterator(d.begin())+1) == utf8_iterator(d.end()));
}

TEST(String, Utf8ICmp)
{
  EXPECT_EQ(-1, utf8_icmp("a", "b"));
  EXPECT_EQ(-1, utf8_icmp("a", "b", 1));
  EXPECT_EQ(-1, utf8_icmp("a", "b", 2));
  EXPECT_EQ(-1, utf8_icmp("a", "aa"));
  EXPECT_EQ(-1, utf8_icmp("A", "aa", 3));
  EXPECT_EQ(-1, utf8_icmp("a", "ab"));

  EXPECT_EQ(0, utf8_icmp("AaE", "aae"));
  EXPECT_EQ(0, utf8_icmp("AaE", "aae", 3));
  EXPECT_EQ(0, utf8_icmp("a", "aa", 1));
  EXPECT_EQ(0, utf8_icmp("a", "ab", 1));

  EXPECT_EQ(1, utf8_icmp("aaa", "Aa", 3));
  EXPECT_EQ(1, utf8_icmp("Bb", "b"));
  EXPECT_EQ(1, utf8_icmp("z", "b"));
  EXPECT_EQ(1, utf8_icmp("z", "b", 1));
  EXPECT_EQ(1, utf8_icmp("z", "b", 2));
}

TEST(String, StringToLowerByUnicodeCharIssue1065)
{
  // Required to make old string_to_lower() version fail.
  std::setlocale(LC_ALL, "en-US");

  std::string  a = "\xC2\xBA";
  std::wstring b = from_utf8(a);
  std::string  c = to_utf8(b);

  ASSERT_EQ(a, c);
  ASSERT_EQ("\xC2\xBA", c);

  ASSERT_EQ(1, utf8_length(a));
  ASSERT_EQ(1, b.size());
  ASSERT_EQ(1, utf8_length(c));

  std::string d = string_to_lower(c);
  ASSERT_EQ(a, d);
  ASSERT_EQ(c, d);
  ASSERT_EQ(1, utf8_length(d));

  auto it = utf8_iterator(d.begin());
  auto end = utf8_iterator(d.end());
  int i = 0;
  for (; it != end; ++it) {
    ASSERT_EQ(b[i++], *it);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
