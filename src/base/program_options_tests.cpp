// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/program_options.h"

using namespace base;

TEST(ProgramOptions, OptionMembers)
{
  ProgramOptions po;
  ProgramOptions::Option& help =
    po.add("help").mnemonic('h').description("Show the help");
  ProgramOptions::Option& output =
    po.add("output").mnemonic('O').requiresValue("OUTPUT");

  EXPECT_EQ("help", help.name());
  EXPECT_EQ("Show the help", help.description());
  EXPECT_EQ('h', help.mnemonic());
  EXPECT_FALSE(po.enabled(help));
  EXPECT_FALSE(help.doesRequireValue());

  EXPECT_EQ("output", output.name());
  EXPECT_EQ("", output.description());
  EXPECT_EQ('O', output.mnemonic());
  EXPECT_FALSE(po.enabled(output));
  EXPECT_TRUE(output.doesRequireValue());
}

TEST(ProgramOptions, Reset)
{
  ProgramOptions po;
  ProgramOptions::Option& help = po.add("help");
  ProgramOptions::Option& file = po.add("file").requiresValue("FILE");
  EXPECT_FALSE(po.enabled(help));
  EXPECT_FALSE(po.enabled(file));
  EXPECT_EQ("", po.value_of(file));

  const char* argv[] = { "program.exe", "--help", "--file=readme.txt" };
  po.parse(3, argv);
  EXPECT_TRUE(po.enabled(help));
  EXPECT_TRUE(po.enabled(file));
  EXPECT_EQ("readme.txt", po.value_of(file));

  po.reset();
  EXPECT_FALSE(po.enabled(help));
  EXPECT_FALSE(po.enabled(file));
  EXPECT_EQ("", po.value_of(file));
}

TEST(ProgramOptions, Parse)
{
  ProgramOptions po;
  ProgramOptions::Option& help = po.add("help").mnemonic('?');
  ProgramOptions::Option& input = po.add("input").mnemonic('i').requiresValue("INPUT");
  ProgramOptions::Option& output = po.add("output").mnemonic('o').requiresValue("OUTPUT");

  const char* argv1[] = { "program.exe", "-?" };
  po.parse(2, argv1);
  EXPECT_TRUE(po.enabled(help));

  const char* argv2[] = { "program.exe", "--help" };
  po.reset();
  po.parse(2, argv2);
  EXPECT_TRUE(po.enabled(help));

  const char* argv3[] = { "program.exe", "--input", "hello.cpp", "--output", "hello.exe" };
  po.reset();
  po.parse(5, argv3);
  EXPECT_FALSE(po.enabled(help));
  EXPECT_TRUE(po.enabled(input));
  EXPECT_TRUE(po.enabled(output));
  EXPECT_EQ("hello.cpp", po.value_of(input));
  EXPECT_EQ("hello.exe", po.value_of(output));

  const char* argv4[] = { "program.exe", "--input=hi.c", "--output=out.exe" };
  po.reset();
  po.parse(3, argv4);
  EXPECT_FALSE(po.enabled(help));
  EXPECT_TRUE(po.enabled(input));
  EXPECT_TRUE(po.enabled(output));
  EXPECT_EQ("hi.c", po.value_of(input));
  EXPECT_EQ("out.exe", po.value_of(output));

  const char* argv5[] = { "program.exe", "-?i", "input.md", "-o", "output.html", "extra-file.txt" };
  po.reset();
  po.parse(6, argv5);
  EXPECT_TRUE(po.enabled(help));
  EXPECT_TRUE(po.enabled(input));
  EXPECT_TRUE(po.enabled(output));
  EXPECT_EQ("input.md", po.value_of(input));
  EXPECT_EQ("output.html", po.value_of(output));
  ASSERT_EQ(4, po.values().size());
  EXPECT_EQ(&help, po.values()[0].option());
  EXPECT_EQ(&input, po.values()[1].option());
  EXPECT_EQ(&output, po.values()[2].option());
  EXPECT_EQ(NULL, po.values()[3].option());
  EXPECT_EQ("", po.values()[0].value());
  EXPECT_EQ("input.md", po.values()[1].value());
  EXPECT_EQ("output.html", po.values()[2].value());
  EXPECT_EQ("extra-file.txt", po.values()[3].value());

  const char* argv6[] = { "program.exe", "value1", "value2", "-o", "output", "value3", "--input=input", "value4" };
  po.reset();
  po.parse(8, argv6);
  ASSERT_EQ(6, po.values().size());
  EXPECT_EQ("value1", po.values()[0].value());
  EXPECT_EQ("value2", po.values()[1].value());
  EXPECT_EQ("output", po.values()[2].value());
  EXPECT_EQ("value3", po.values()[3].value());
  EXPECT_EQ("input", po.values()[4].value());
  EXPECT_EQ("value4", po.values()[5].value());
}

TEST(ProgramOptions, ParseErrors)
{
  ProgramOptions po;
  ProgramOptions::Option& help = po.add("help").mnemonic('?');
  ProgramOptions::Option& input = po.add("input").mnemonic('i').requiresValue("INPUT");
  ProgramOptions::Option& output = po.add("output").mnemonic('o').requiresValue("OUTPUT");

  const char* argv1[] = { "program.exe", "--input" };
  EXPECT_THROW(po.parse(2, argv1), ProgramOptionNeedsValue);

  const char* argv2[] = { "program.exe", "-i" };
  EXPECT_THROW(po.parse(2, argv2), ProgramOptionNeedsValue);

  const char* argv3[] = { "program.exe", "--test" };
  EXPECT_THROW(po.parse(2, argv3), InvalidProgramOption);

  const char* argv4[] = { "program.exe", "-?a" };
  po.reset();
  EXPECT_FALSE(po.enabled(help));
  EXPECT_THROW(po.parse(2, argv4), InvalidProgramOption);
  EXPECT_TRUE(po.enabled(help));  // -? is parsed anyway, -a is the invalid option

  const char* argv5[] = { "program.exe", "-io", "input-and-output.txt" };
  po.reset();
  EXPECT_THROW(po.parse(2, argv5), ProgramOptionNeedsValue);
  po.reset();
  EXPECT_THROW(po.parse(3, argv5), InvalidProgramOptionsCombination);
  EXPECT_TRUE(po.enabled(input));
  EXPECT_FALSE(po.enabled(output));
  EXPECT_EQ("input-and-output.txt", po.value_of(input));
  EXPECT_EQ("", po.value_of(output));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
