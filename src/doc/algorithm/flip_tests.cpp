// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gtest/gtest.h"

#include "doc/algorithm/flip_image.h"

#include "doc/algorithm/random_image.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"

using namespace doc;
using namespace gfx;

TEST(Flip, Image)
{
  for (auto pf : { IMAGE_RGB, IMAGE_GRAYSCALE, IMAGE_INDEXED, IMAGE_BITMAP, IMAGE_TILEMAP }) {
    for (int h = 2; h < 200; h += 5) {
      for (int w = 2; w < 200; w += 5) {
        ImageRef a(Image::create(pf, w, h));
        doc::algorithm::random_image(a.get());

        // random_image() shouldn't generate a plain image
        // ASSERT_FALSE(is_plain_image(a.get(), get_pixel(a.get(), 0, 0)));

        ImageRef b(Image::createCopy(a.get()));
        ImageRef c(Image::createCopy(a.get()));
        ASSERT_TRUE(is_same_image(b.get(), c.get()));

        for (auto ft : { doc::algorithm::FlipHorizontal, doc::algorithm::FlipVertical }) {
          doc::algorithm::flip_image(b.get(), b->bounds(), ft);
          doc::algorithm::flip_image_slow(c.get(), c->bounds(), ft);

          ASSERT_TRUE(is_same_image(b.get(), c.get()))
            << "Pixel format=" << pf << " Size=" << w << "x" << h << "\nFlip type=" << ft;

          doc::algorithm::flip_image(b.get(), b->bounds(), ft);
          doc::algorithm::flip_image_slow(c.get(), c->bounds(), ft);

          ASSERT_TRUE(is_same_image(a.get(), b.get()));
          ASSERT_TRUE(is_same_image(a.get(), c.get()));
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
