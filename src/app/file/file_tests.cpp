// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/app.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "base/base64.h"
#include "doc/doc.h"
#include "doc/user_data.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
#include <fstream>

using namespace app;

TEST(File, SeveralSizes)
{
  // Register all possible image formats.
  std::vector<char> fn(256);
  app::Context ctx;

  for (int w=10; w<=10+503*2; w+=503) {
    for (int h=10; h<=10+503*2; h+=503) {
      //std::sprintf(&fn[0], "test_%dx%d.ase", w, h);
      std::sprintf(&fn[0], "test.ase");

      {
        std::unique_ptr<Doc> doc(
          ctx.documents().add(w, h, doc::ColorMode::INDEXED, 256));
        doc->setFilename(&fn[0]);

        // Random pixels
        Layer* layer = doc->sprite()->root()->firstLayer();
        ASSERT_TRUE(layer != NULL);
        Image* image = layer->cel(frame_t(0))->image();
        std::srand(w*h);
        int c = std::rand()%256;
        for (int y=0; y<h; y++) {
          for (int x=0; x<w; x++) {
            put_pixel_fast<IndexedTraits>(image, x, y, c);
            if ((std::rand()&4) == 0)
              c = std::rand()%256;
          }
        }

        save_document(&ctx, doc.get());
        doc->close();
      }

      {
        std::unique_ptr<Doc> doc(load_document(&ctx, &fn[0]));
        ASSERT_EQ(w, doc->sprite()->width());
        ASSERT_EQ(h, doc->sprite()->height());

        // Same random pixels (see the seed)
        Layer* layer = doc->sprite()->root()->firstLayer();
        ASSERT_TRUE(layer != nullptr);
        Image* image = layer->cel(frame_t(0))->image();
        std::srand(w*h);
        int c = std::rand()%256;
        for (int y=0; y<h; y++) {
          for (int x=0; x<w; x++) {
            ASSERT_EQ(c, get_pixel_fast<IndexedTraits>(image, x, y));
            if ((std::rand()&4) == 0)
              c = std::rand()%256;
          }
        }

        doc->close();
      }
    }
  }
}

TEST(File, CustomProperties)
{
  app::Context ctx;

  struct TestCase {
    std::string filename;
    int w;
    int h;
    doc::ColorMode mode;
    int ncolors;
    doc::UserData::PropertiesMaps propertiesMaps;
    std::function<void(const TestCase&, doc::Sprite*)> setProperties;
    std::function<void(const TestCase&, doc::Sprite*)> assertions;
  };
  std::vector<TestCase> tests = {
    { // Test sprite's userData simple custom properties
      "test_props_1.ase", 50, 50, doc::ColorMode::INDEXED, 256,
      {
        {"", {
               {"number", int32_t(560304)},
               {"is_solid", bool(true)},
               {"label", std::string("Rock")},
               {"weight", doc::UserData::Fixed{fixmath::ftofix(50.34)}},
               {"big_number", int64_t(9223372036854775807)},
               {"unsigned_big_number", uint64_t(18446744073709551615ULL)},
               {"my_uuid", base::Uuid::Generate()},
             }
        }
      },
      [](const TestCase& test, doc::Sprite* sprite){
        sprite->userData().propertiesMaps() = test.propertiesMaps;
      },
      [](const TestCase& test, doc::Sprite* sprite){
        ASSERT_EQ(doc::get_value<int32_t>(sprite->userData().properties()["number"]), 560304);
        ASSERT_EQ(doc::get_value<bool>(sprite->userData().properties()["is_solid"]), true);
        ASSERT_EQ(doc::get_value<std::string>(sprite->userData().properties()["label"]), "Rock");
        ASSERT_EQ(doc::get_value<doc::UserData::Fixed>(sprite->userData().properties()["weight"]).value, fixmath::ftofix(50.34));
        ASSERT_EQ(doc::get_value<int64_t>(sprite->userData().properties()["big_number"]), 9223372036854775807);
        ASSERT_EQ(doc::get_value<uint64_t>(sprite->userData().properties()["unsigned_big_number"]), 18446744073709551615ULL);
        ASSERT_EQ(doc::get_value<base::Uuid>(sprite->userData().properties()["my_uuid"]),
                  doc::get_value<base::Uuid>(test.propertiesMaps.at("").at("my_uuid")));
      }
    },
    { // Test sprite's userData extension's simple properties
      "test_props_2.ase", 50, 50, doc::ColorMode::INDEXED, 256,
      {
        {"extensionIdentification", {
               {"number", int32_t(160304)},
               {"is_solid", bool(false)},
               {"label", std::string("Smoke")},
               {"weight", doc::UserData::Fixed{fixmath::ftofix(0.14)}},
               {"my_uuid", base::Uuid::Generate()},
             }
        }
      },
      [](const TestCase& test, doc::Sprite* sprite){
        sprite->userData().propertiesMaps() = test.propertiesMaps;
      },
      [](const TestCase& test, doc::Sprite* sprite){
        ASSERT_EQ(doc::get_value<int32_t>(sprite->userData().properties("extensionIdentification")["number"]), 160304);
        ASSERT_EQ(doc::get_value<bool>(sprite->userData().properties("extensionIdentification")["is_solid"]), false);
        ASSERT_EQ(doc::get_value<std::string>(sprite->userData().properties("extensionIdentification")["label"]), "Smoke");
        ASSERT_EQ(doc::get_value<doc::UserData::Fixed>(sprite->userData().properties("extensionIdentification")["weight"]).value, fixmath::ftofix(0.14));
        ASSERT_EQ(doc::get_value<base::Uuid>(sprite->userData().properties("extensionIdentification")["my_uuid"]),
                                             doc::get_value<base::Uuid>(test.propertiesMaps.at("extensionIdentification").at("my_uuid")));
      }
    },
    { // Test sprite's userData custom + extension's simple properties
      "test_props_3.ase", 50, 50, doc::ColorMode::INDEXED, 256,
      {
        {"", {
               {"number", int32_t(560304)},
               {"is_solid", bool(true)},
               {"label", std::string("Rock")},
               {"weight", doc::UserData::Fixed{fixmath::ftofix(50.34)}},
               {"my_uuid", base::Uuid::Generate()},
             }
        },
        {"extensionIdentification", {
               {"number", int32_t(160304)},
               {"is_solid", bool(false)},
               {"label", std::string("Smoke")},
               {"weight", doc::UserData::Fixed{fixmath::ftofix(0.14)}},
               {"my_uuid2", base::Uuid::Generate()},
             }
        }
      },
      [](const TestCase& test, doc::Sprite* sprite){
        sprite->userData().propertiesMaps() = test.propertiesMaps;
      },
      [](const TestCase& test, doc::Sprite* sprite){
        ASSERT_EQ(doc::get_value<int32_t>(sprite->userData().properties()["number"]), 560304);
        ASSERT_EQ(doc::get_value<bool>(sprite->userData().properties()["is_solid"]), true);
        ASSERT_EQ(doc::get_value<std::string>(sprite->userData().properties()["label"]), "Rock");
        ASSERT_EQ(doc::get_value<doc::UserData::Fixed>(sprite->userData().properties()["weight"]).value, fixmath::ftofix(50.34));
        ASSERT_EQ(doc::get_value<base::Uuid>(sprite->userData().properties("")["my_uuid"]),
                                             doc::get_value<base::Uuid>(test.propertiesMaps.at("").at("my_uuid")));

        ASSERT_EQ(doc::get_value<int32_t>(sprite->userData().properties("extensionIdentification")["number"]), 160304);
        ASSERT_EQ(doc::get_value<bool>(sprite->userData().properties("extensionIdentification")["is_solid"]), false);
        ASSERT_EQ(doc::get_value<std::string>(sprite->userData().properties("extensionIdentification")["label"]), "Smoke");
        ASSERT_EQ(doc::get_value<doc::UserData::Fixed>(sprite->userData().properties("extensionIdentification")["weight"]).value, fixmath::ftofix(0.14));
        ASSERT_EQ(doc::get_value<base::Uuid>(sprite->userData().properties("extensionIdentification")["my_uuid2"]),
                                             doc::get_value<base::Uuid>(test.propertiesMaps.at("extensionIdentification").at("my_uuid2")));
      }
    },
    { // Test sprite's userData complex properties
      "test_props_4.ase", 50, 50, doc::ColorMode::INDEXED, 256,
      {
        {"", {
               {"coordinates", gfx::Point(10, 20)},
               {"size", gfx::Size(100, 200)},
               {"bounds", gfx::Rect(30, 40, 150, 250)},
               {"items", doc::UserData::Vector {
                  std::string("arrow"), std::string("hammer"), std::string("coin")
               }},
               {"player", doc::UserData::Properties {
                            {"lives", uint8_t(5)},
                            {"name", std::string("John Doe")},
                            {"energy", uint16_t(1000)}
                          }
               }
             }
        },
        {"ext", {
                  {"numbers", doc::UserData::Vector {int32_t(11), int32_t(22), int32_t(33)}},
                  {"player", doc::UserData::Properties {
                                {"id", uint32_t(12347455)},
                                {"coordinates", gfx::Point(45, 56)},
                                {"cards", doc::UserData::Vector {int8_t(11), int8_t(6), int8_t(0), int8_t(13)}}
                              }
                  }
                }
        }
      },
      [](const TestCase& test, doc::Sprite* sprite){
        sprite->userData().propertiesMaps() = test.propertiesMaps;
      },
      [](const TestCase& test, doc::Sprite* sprite){
        ASSERT_EQ(doc::get_value<gfx::Point>(sprite->userData().properties()["coordinates"]),
                  gfx::Point(10, 20));
        ASSERT_EQ(doc::get_value<gfx::Size>(sprite->userData().properties()["size"]),
                  gfx::Size(100, 200));
        ASSERT_EQ(doc::get_value<gfx::Rect>(sprite->userData().properties()["bounds"]),
                  gfx::Rect(30, 40, 150, 250));
        ASSERT_EQ(doc::get_value<doc::UserData::Vector>(sprite->userData().properties()["items"]),
                  (doc::UserData::Vector{std::string("arrow"), std::string("hammer"), std::string("coin")}));
        ASSERT_EQ(doc::get_value<doc::UserData::Properties>(sprite->userData().properties()["player"]),
                  (doc::UserData::Properties {
                    {"lives", uint8_t(5)},
                    {"name", std::string("John Doe")},
                    {"energy", uint16_t(1000)}
                  }));

        ASSERT_EQ(doc::get_value<doc::UserData::Vector>(sprite->userData().properties("ext")["numbers"]),
                  (doc::UserData::Vector {int8_t(11), int8_t(22), int8_t(33)}));

        ASSERT_EQ(doc::get_value<doc::UserData::Properties>(sprite->userData().properties("ext")["player"]),
                  (doc::UserData::Properties {
                    {"id", uint32_t(12347455)},
                    {"coordinates", gfx::Point(45, 56)},
                    {"cards", doc::UserData::Vector {int8_t(11), int8_t(6), int8_t(0), int8_t(13)}}
                  }));
      }
    },
    { // Test size reduction of integer properties
      "test_props_4.ase", 50, 50, doc::ColorMode::INDEXED, 256,
      {
        {"", {
               {"int16_to_int8", int16_t(127)},
               {"int16_to_uint8", int16_t(128)},
               {"int32_to_int8", int32_t(126)},
               {"int32_to_uint8", int32_t(129)},
               {"int32_to_int16", int32_t(32767)},
               {"int32_to_uint16", int32_t(32768)},
               {"int64_to_int8", int64_t(125)},
               {"int64_to_uint8", int64_t(130)},
               {"int64_to_int16", int64_t(32765)},
               {"int64_to_uint16", int64_t(32769)},
               {"int64_to_int32", int64_t(2147483647)},
               {"int64_to_uint32", int64_t(2147483648)},
               {"v1", doc::UserData::Vector {uint64_t(18446744073709551615ULL), uint64_t(6), uint64_t(0), uint64_t(13)}},
             }
        }
      },
      [](const TestCase& test, doc::Sprite* sprite){
        sprite->userData().propertiesMaps() = test.propertiesMaps;
      },
      [](const TestCase& test, doc::Sprite* sprite){
        ASSERT_EQ(doc::get_value<int8_t>(sprite->userData().properties()["int16_to_int8"]), 127);
        ASSERT_EQ(doc::get_value<uint8_t>(sprite->userData().properties()["int16_to_uint8"]), 128);
        ASSERT_EQ(doc::get_value<int8_t>(sprite->userData().properties()["int32_to_int8"]), 126);
        ASSERT_EQ(doc::get_value<uint8_t>(sprite->userData().properties()["int32_to_uint8"]), 129);
        ASSERT_EQ(doc::get_value<int16_t>(sprite->userData().properties()["int32_to_int16"]), 32767);
        ASSERT_EQ(doc::get_value<uint16_t>(sprite->userData().properties()["int32_to_uint16"]), 32768);
        ASSERT_EQ(doc::get_value<int8_t>(sprite->userData().properties()["int64_to_int8"]), 125);
        ASSERT_EQ(doc::get_value<uint8_t>(sprite->userData().properties()["int64_to_uint8"]), 130);
        ASSERT_EQ(doc::get_value<int16_t>(sprite->userData().properties()["int64_to_int16"]), 32765);
        ASSERT_EQ(doc::get_value<uint16_t>(sprite->userData().properties()["int64_to_uint16"]), 32769);
        ASSERT_EQ(doc::get_value<int32_t>(sprite->userData().properties()["int64_to_int32"]), 2147483647);
        ASSERT_EQ(doc::get_value<uint32_t>(sprite->userData().properties()["int64_to_uint32"]), 2147483648);
        ASSERT_EQ(doc::get_value<doc::UserData::Vector>(sprite->userData().properties()["v1"]),
                  (doc::UserData::Vector {uint64_t(18446744073709551615ULL), uint64_t(6), uint64_t(0), uint64_t(13)}));
      }
    }
  };

  for (const TestCase& test : tests) {
    {
      std::unique_ptr<Doc> doc(
        ctx.documents().add(test.w, test.h, test.mode, test.ncolors));
      doc->setFilename(test.filename);
      // Random pixels
      LayerImage* layer = static_cast<LayerImage*>(doc->sprite()->root()->firstLayer());
      ASSERT_TRUE(layer != NULL);
      ImageRef image = layer->cel(frame_t(0))->imageRef();

      std::srand(test.w*test.h);
      int c = 0;
      for (int y=0; y<test.h; y++) {
        for (int x=0; x<test.w; x++) {
          if ((std::rand()&4) == 0)
            c = std::rand()%test.ncolors;
          put_pixel_fast<IndexedTraits>(image.get(), x, y, c);
        }
      }

      test.setProperties(test, doc->sprite());

      save_document(&ctx, doc.get());
      doc->close();
    }
    {
      std::unique_ptr<Doc> doc(load_document(&ctx, test.filename));
      ASSERT_EQ(test.w, doc->sprite()->width());
      ASSERT_EQ(test.h, doc->sprite()->height());

      // Same random pixels (see the seed)
      Layer* layer = doc->sprite()->root()->firstLayer();
      ASSERT_TRUE(layer != nullptr);
      Image* image = layer->cel(frame_t(0))->image();
      std::srand(test.w*test.h);
      int c = 0;
      for (int y=0; y<test.h; y++) {
        for (int x=0; x<test.w; x++) {
          if ((std::rand()&4) == 0)
            c = std::rand()%test.ncolors;
          ASSERT_EQ(c, get_pixel_fast<IndexedTraits>(image, x, y));
        }
      }

      test.assertions(test, doc->sprite());

      doc->close();
    }
  }
}
