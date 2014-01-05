// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

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
  EXPECT_FALSE(help.enabled());
  EXPECT_FALSE(help.doesRequireValue());

  EXPECT_EQ("output", output.name());
  EXPECT_EQ("", output.description());
  EXPECT_EQ('O', output.mnemonic());
  EXPECT_FALSE(output.enabled());
  EXPECT_TRUE(output.doesRequireValue());
}

TEST(ProgramOptions, Reset)
{
  ProgramOptions po;
  ProgramOptions::Option& help = po.add("help");
  ProgramOptions::Option& file = po.add("file").requiresValue("FILE");
  EXPECT_FALSE(help.enabled());
  EXPECT_FALSE(file.enabled());
  EXPECT_EQ("", file.value());

  const char* argv[] = { "program.exe", "--help", "--file=readme.txt" };
  po.parse(3, argv);
  EXPECT_TRUE(help.enabled());
  EXPECT_TRUE(file.enabled());
  EXPECT_EQ("readme.txt", file.value());

  po.reset();
  EXPECT_FALSE(help.enabled());
  EXPECT_FALSE(file.enabled());
  EXPECT_EQ("", file.value());
}

TEST(ProgramOptions, Parse)
{
  ProgramOptions po;
  ProgramOptions::Option& help = po.add("help").mnemonic('?');
  ProgramOptions::Option& input = po.add("input").mnemonic('i').requiresValue("INPUT");
  ProgramOptions::Option& output = po.add("output").mnemonic('o').requiresValue("OUTPUT");

  const char* argv1[] = { "program.exe", "-?" };
  po.parse(2, argv1);
  EXPECT_TRUE(help.enabled());

  const char* argv2[] = { "program.exe", "--help" };
  po.reset();
  po.parse(2, argv2);
  EXPECT_TRUE(help.enabled());

  const char* argv3[] = { "program.exe", "--input", "hello.cpp", "--output", "hello.exe" };
  po.reset();
  po.parse(5, argv3);
  EXPECT_FALSE(help.enabled());
  EXPECT_TRUE(input.enabled());
  EXPECT_TRUE(output.enabled());
  EXPECT_EQ("hello.cpp", input.value());
  EXPECT_EQ("hello.exe", output.value());

  const char* argv4[] = { "program.exe", "--input=hi.c", "--output=out.exe" };
  po.reset();
  po.parse(3, argv4);
  EXPECT_FALSE(help.enabled());
  EXPECT_TRUE(input.enabled());
  EXPECT_TRUE(output.enabled());
  EXPECT_EQ("hi.c", input.value());
  EXPECT_EQ("out.exe", output.value());

  const char* argv5[] = { "program.exe", "-?i", "input.md", "-o", "output.html", "extra-file.txt" };
  po.reset();
  po.parse(6, argv5);
  EXPECT_TRUE(help.enabled());
  EXPECT_TRUE(input.enabled());
  EXPECT_TRUE(output.enabled());
  EXPECT_EQ("input.md", input.value());
  EXPECT_EQ("output.html", output.value());
  ASSERT_EQ(1, po.values().size());
  EXPECT_EQ("extra-file.txt", po.values()[0]);

  const char* argv6[] = { "program.exe", "value1", "value2", "-o", "output", "value3", "--input=input", "value4" };
  po.reset();
  po.parse(8, argv6);
  ASSERT_EQ(4, po.values().size());
  EXPECT_EQ("value1", po.values()[0]);
  EXPECT_EQ("value2", po.values()[1]);
  EXPECT_EQ("value3", po.values()[2]);
  EXPECT_EQ("value4", po.values()[3]);
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
  EXPECT_FALSE(help.enabled());
  EXPECT_THROW(po.parse(2, argv4), InvalidProgramOption);
  EXPECT_TRUE(help.enabled());  // -? is parsed anyway, -a is the invalid option

  const char* argv5[] = { "program.exe", "-io", "input-and-output.txt" };
  po.reset();
  EXPECT_THROW(po.parse(2, argv5), ProgramOptionNeedsValue);
  po.reset();
  EXPECT_THROW(po.parse(3, argv5), InvalidProgramOptionsCombination);
  EXPECT_TRUE(input.enabled());
  EXPECT_TRUE(output.enabled());
  EXPECT_EQ("input-and-output.txt", input.value());
  EXPECT_EQ("", output.value());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
