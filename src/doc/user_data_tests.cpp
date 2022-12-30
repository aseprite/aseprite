// Aseprite Document Library
// Copyright (c) 2022 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/user_data.h"

using namespace doc;
using Variant = UserData::Variant;
using Fixed = UserData::Fixed;
using Vector = UserData::Vector;
using Properties = UserData::Properties;

TEST(CustomProperties, SimpleProperties)
{
  UserData data;
  // Initial data doesn't have any properties
  EXPECT_TRUE(data.properties().empty());

  data.properties()["boolean"] = false;
  data.properties()["char"] = uint8_t('A');
  data.properties()["number16"] = int16_t(-1024);
  data.properties()["number32"] = int32_t(-5628102);
  data.properties()["text"] = std::string("this is some text");
  EXPECT_TRUE(data.properties().size() == 5);

  bool boolean = get_value<bool>(data.properties()["boolean"]);
  EXPECT_FALSE(boolean);

  uint8_t charac = get_value<uint8_t>(data.properties()["char"]);
  EXPECT_EQ('A', charac);

  int16_t number16 = get_value<int16_t>(data.properties()["number16"]);
  EXPECT_EQ(-1024, number16);

  int32_t number32 = get_value<int32_t>(data.properties()["number32"]);
  EXPECT_EQ(-5628102, number32);

  std::string text = get_value<std::string>(data.properties()["text"]);
  EXPECT_EQ("this is some text", text);
}

TEST(CustomProperties, ComplexProperties)
{
  UserData data;
  // Add a vector of ints as the "list" custom property.
  data.properties()["list"] = Vector{ 1, 2, 3 };
  // Check that we have one property
  EXPECT_TRUE(data.properties().size() == 1);

  // Get the "list" vector and check some of its content
  auto list = get_value<Vector>(data.properties()["list"]);
  EXPECT_TRUE(list.size() == 3);
  int v1 = get_value<int>(list[1]);
  EXPECT_EQ(2, v1);

  // Add Point, Size, and Rect properties
  data.properties()["point"] = gfx::Point(10,30);
  data.properties()["size"] = gfx::Size(50,20);
  data.properties()["rect"] = gfx::Rect(11,22,33,44);
  EXPECT_EQ(4, data.properties().size());

  gfx::Point point = get_value<gfx::Point>(data.properties()["point"]);
  EXPECT_TRUE(point.x == 10 && point.y == 30);

  gfx::Size size = get_value<gfx::Size>(data.properties()["size"]);
  EXPECT_TRUE(size.w == 50 && size.h == 20);

  gfx::Rect rect = get_value<gfx::Rect>(data.properties()["rect"]);
  EXPECT_TRUE(rect.x == 11 && rect.y == 22);
  EXPECT_TRUE(rect.w == 33 && rect.h == 44);

  // Add Fixed property
  data.properties()["fixed"] = Fixed{fixmath::ftofix(10.5)};
  EXPECT_EQ(5, data.properties().size());

  Fixed fixed = get_value<Fixed>(data.properties()["fixed"]);
  EXPECT_EQ(fixmath::ftofix(10.5), fixed.value);

  // Add an object with Properties
  data.properties()["object"] = Properties{ { "id", uint16_t(400) },
                                            { "color", std::string("red") },
                                            { "size", gfx::Size{ 25, 65 } } };
  EXPECT_TRUE(data.properties().size() == 6);

  Properties object = get_value<Properties>(data.properties()["object"]);
  uint16_t id = get_value<uint16_t>(object["id"]);
  EXPECT_EQ(400, id);

  std::string color = get_value<std::string>(object["color"]);
  EXPECT_EQ("red", color);

  size = get_value<gfx::Size>(object["size"]);
  EXPECT_TRUE(size.w == 25 && size.h == 65);

  // Try to get wrong type
  gfx::Point* p = std::get_if<gfx::Point>(&data.properties()["rect"]);
  EXPECT_EQ(nullptr, p);

  data.properties().erase("rect");
  data.properties().erase("list");
  data.properties().erase("point");
  data.properties().erase("size");
  data.properties().erase("object");
  data.properties().erase("fixed");
  EXPECT_EQ(0, data.properties().size());
}

TEST(ExtensionProperties, SimpleProperties)
{
  UserData data;
  EXPECT_TRUE(data.properties().empty());
  EXPECT_TRUE(data.properties("someExtensionId").empty());

  data.properties("someExtensionId")["boolean"] = false;
  data.properties("someExtensionId")["char"] = uint8_t('A');
  data.properties("someExtensionId")["number16"] = int16_t(-1024);
  data.properties()["number32"] = int32_t(-5628102);
  data.properties()["text"] = std::string("this is some text");
  EXPECT_TRUE(data.properties("someExtensionId").size() == 3);
  EXPECT_TRUE(data.properties().size() == 2);

  auto boolean = get_value<bool>(data.properties("someExtensionId")["boolean"]);
  EXPECT_FALSE(boolean);

  auto charac = get_value<uint8_t>(data.properties("someExtensionId")["char"]);
  EXPECT_EQ('A', charac);

  auto number16 = get_value<int16_t>(data.properties("someExtensionId")["number16"]);
  EXPECT_EQ(-1024, number16);

  auto number32 = get_value<int32_t>(data.properties()["number32"]);
  EXPECT_EQ(-5628102, number32);

  auto text = get_value<std::string>(data.properties()["text"]);
  EXPECT_EQ("this is some text", text);

  data.properties().erase("number32");
  data.properties().erase("text");
  EXPECT_TRUE(data.properties().size() == 0);
  EXPECT_TRUE(data.properties("someExtensionId").size() == 3);

  data.properties("someExtensionId").erase("number16");
  data.properties("someExtensionId").erase("boolean");
  data.properties("someExtensionId").erase("char");
  EXPECT_TRUE(data.properties("someExtensionId").size() == 0);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
