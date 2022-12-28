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
using Properties = UserData::Properties;

TEST(CustomProperties, SimpleProperties)
{
  UserData data;
  // Initial data doesn't have any properties
  EXPECT_TRUE(data.properties().empty());

  data.properties()["boolean"] = Variant{false};
  data.properties()["char"] = Variant{uint8_t('A')};
  data.properties()["number16"] = Variant{int16_t(-1024)};
  data.properties()["number32"] = Variant{int32_t(-5628102)};
  data.properties()["text"] = Variant{std::string("this is some text")};
  EXPECT_TRUE(data.properties().size() == 5);

  bool* boolean = std::get_if<bool>(&data.properties()["boolean"]);
  EXPECT_TRUE(boolean);
  EXPECT_FALSE(*boolean);

  uint8_t* charac = std::get_if<uint8_t>(&data.properties()["char"]);
  EXPECT_TRUE(charac);
  EXPECT_TRUE(*charac == 'A');

  int16_t* number16 = std::get_if<int16_t>(&data.properties()["number16"]);
  EXPECT_TRUE(number16);
  EXPECT_TRUE(*number16 == -1024);

  int32_t* number32 = std::get_if<int32_t>(&data.properties()["number32"]);
  EXPECT_TRUE(number32);
  EXPECT_TRUE(*number32 == -5628102);

  std::string* text = std::get_if<std::string>(&data.properties()["text"]);
  EXPECT_TRUE(text);
  EXPECT_TRUE(*text == "this is some text");
}

TEST(CustomProperties, ComplexProperties)
{
  UserData data;
  // Add a vector of ints as the "list" custom property.
  data.properties()["list"] = Variant{
    std::vector<Variant>{
      Variant{1},
      Variant{2},
      Variant{3}
    }
  };
  // Check that we have one property
  EXPECT_TRUE(data.properties().size() == 1);

  // Get the "list" vector and check some of its content
  std::vector<Variant>* list = std::get_if<std::vector<Variant>>(&data.properties()["list"]);
  EXPECT_TRUE(list);
  EXPECT_TRUE(list->size() == 3);
  int* v1 = std::get_if<int>(&(*list)[1]);
  EXPECT_TRUE(*v1 == 2);

  // Add Point, Size, and Rect properties
  data.properties()["point"] = Variant{gfx::Point(10,30)};
  data.properties()["size"] = Variant{gfx::Size(50,20)};
  data.properties()["rect"] = Variant{gfx::Rect(11,22,33,44)};
  EXPECT_TRUE(data.properties().size() == 4);

  gfx::Point* point = std::get_if<gfx::Point>(&data.properties()["point"]);
  EXPECT_TRUE(point);
  EXPECT_TRUE(point->x == 10 && point->y == 30);

  gfx::Size* size = std::get_if<gfx::Size>(&data.properties()["size"]);
  EXPECT_TRUE(size);
  EXPECT_TRUE(size->w == 50 && size->h == 20);

  gfx::Rect* rect = std::get_if<gfx::Rect>(&data.properties()["rect"]);
  EXPECT_TRUE(rect);
  EXPECT_TRUE(rect->x == 11 && rect->y == 22);
  EXPECT_TRUE(rect->w == 33 && rect->h == 44);

  // Add Fixed property
  data.properties()["fixed"] = Variant{Fixed{fixmath::ftofix(10.5)}};
  EXPECT_TRUE(data.properties().size() == 5);

  Fixed* fixed = std::get_if<Fixed>(&data.properties()["fixed"]);
  EXPECT_TRUE(fixed);
  EXPECT_TRUE(fixed->value == fixmath::ftofix(10.5));

  // Add an object with Properties
  data.properties()["object"] = Variant{
    Properties{
      {"id", Variant{uint16_t(400)}},
      {"color", Variant{std::string("red")}},
      {"size", Variant{gfx::Size{25,65}}}
    }
  };
  EXPECT_TRUE(data.properties().size() == 6);

  Properties* object = std::get_if<Properties>(&data.properties()["object"]);
  EXPECT_TRUE(object);

  uint16_t* id = std::get_if<uint16_t>(&(*object)["id"]);
  EXPECT_TRUE(id);
  EXPECT_TRUE(*id == 400);

  std::string* color = std::get_if<std::string>(&(*object)["color"]);
  EXPECT_TRUE(color);
  EXPECT_TRUE(*color == "red");

  size = std::get_if<gfx::Size>(&(*object)["size"]);
  EXPECT_TRUE(size);
  EXPECT_TRUE(size->w == 25 && size->h == 65);

  // Try to get wrong type
  gfx::Point* p = std::get_if<gfx::Point>(&data.properties()["rect"]);
  EXPECT_TRUE(p == nullptr);

  data.properties().erase("rect");
  data.properties().erase("list");
  data.properties().erase("point");
  data.properties().erase("size");
  data.properties().erase("object");
  data.properties().erase("fixed");
  EXPECT_TRUE(data.properties().size() == 0);
}

TEST(ExtensionProperties, SimpleProperties)
{
  UserData data;
  EXPECT_TRUE(data.properties().empty());
  EXPECT_TRUE(data.properties("someExtensionId").empty());

  data.properties("someExtensionId")["boolean"] = Variant{false};
  data.properties("someExtensionId")["char"] = Variant{uint8_t('A')};
  data.properties("someExtensionId")["number16"] = Variant{int16_t(-1024)};
  data.properties()["number32"] = Variant{int32_t(-5628102)};
  data.properties()["text"] = Variant{std::string("this is some text")};
  EXPECT_TRUE(data.properties("someExtensionId").size() == 3);
  EXPECT_TRUE(data.properties().size() == 2);

  bool* boolean = std::get_if<bool>(&data.properties("someExtensionId")["boolean"]);
  EXPECT_TRUE(boolean);
  EXPECT_FALSE(*boolean);

  uint8_t* charac = std::get_if<uint8_t>(&data.properties("someExtensionId")["char"]);
  EXPECT_TRUE(charac);
  EXPECT_TRUE(*charac == 'A');

  int16_t* number16 = std::get_if<int16_t>(&data.properties("someExtensionId")["number16"]);
  EXPECT_TRUE(number16);
  EXPECT_TRUE(*number16 == -1024);

  int32_t* number32 = std::get_if<int32_t>(&data.properties()["number32"]);
  EXPECT_TRUE(number32);
  EXPECT_TRUE(*number32 == -5628102);

  std::string* text = std::get_if<std::string>(&data.properties()["text"]);
  EXPECT_TRUE(text);
  EXPECT_TRUE(*text == "this is some text");

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
