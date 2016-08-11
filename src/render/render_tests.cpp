// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "render/render.h"

#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/context.h"
#include "doc/document.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"

using namespace doc;
using namespace render;

template<typename T>
class RenderAllModes : public testing::Test {
protected:
  RenderAllModes() { }
};

typedef testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits> ImageAllTraits;
TYPED_TEST_CASE(RenderAllModes, ImageAllTraits);

// a b
// c d
#define EXPECT_2X2_PIXELS(image, a, b, c, d) \
  EXPECT_EQ(a, get_pixel(image, 0, 0));      \
  EXPECT_EQ(b, get_pixel(image, 1, 0));      \
  EXPECT_EQ(c, get_pixel(image, 0, 1));      \
  EXPECT_EQ(d, get_pixel(image, 1, 1))

// a b c d
// e f g h
// i j k l
// m n o p
#define EXPECT_4X4_PIXELS(image, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  EXPECT_EQ(a, get_pixel(image, 0, 0));                                 \
  EXPECT_EQ(b, get_pixel(image, 1, 0));                                 \
  EXPECT_EQ(c, get_pixel(image, 2, 0));                                 \
  EXPECT_EQ(d, get_pixel(image, 3, 0));                                 \
  EXPECT_EQ(e, get_pixel(image, 0, 1));                                 \
  EXPECT_EQ(f, get_pixel(image, 1, 1));                                 \
  EXPECT_EQ(g, get_pixel(image, 2, 1));                                 \
  EXPECT_EQ(h, get_pixel(image, 3, 1));                                 \
  EXPECT_EQ(i, get_pixel(image, 0, 2));                                 \
  EXPECT_EQ(j, get_pixel(image, 1, 2));                                 \
  EXPECT_EQ(k, get_pixel(image, 2, 2));                                 \
  EXPECT_EQ(l, get_pixel(image, 3, 2));                                 \
  EXPECT_EQ(m, get_pixel(image, 0, 3));                                 \
  EXPECT_EQ(n, get_pixel(image, 1, 3));                                 \
  EXPECT_EQ(o, get_pixel(image, 2, 3));                                 \
  EXPECT_EQ(p, get_pixel(image, 3, 3))

TEST(Render, Basic)
{
  Context ctx;
  Document* doc = ctx.documents().add(2, 2, ColorMode::RGB);

  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 2);

  base::UniquePtr<Image> dst(Image::create(IMAGE_RGB, 2, 2));
  clear_image(dst, 1);
  EXPECT_2X2_PIXELS(dst, 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_2X2_PIXELS(dst, 2, 2, 2, 2);
}

TYPED_TEST(RenderAllModes, CheckDefaultBackgroundMode)
{
  typedef TypeParam ImageTraits;

  Context ctx;
  Document* doc = ctx.documents().add(2, 2,
    ColorMode(ImageTraits::pixel_format));

  EXPECT_TRUE(!doc->sprite()->root()->firstLayer()->isBackground());
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 0);
  put_pixel(src, 1, 1, 1);

  base::UniquePtr<Image> dst(Image::create(ImageTraits::pixel_format, 2, 2));
  clear_image(dst, 1);
  EXPECT_2X2_PIXELS(dst, 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  // Default background mode is to set all pixels to transparent color
  EXPECT_2X2_PIXELS(dst, 0, 0, 0, 1);
}

TEST(Render, DefaultBackgroundModeWithNonzeroTransparentIndex)
{
  Context ctx;
  Document* doc = ctx.documents().add(2, 2, ColorMode::INDEXED);
  doc->sprite()->setTransparentColor(2); // Transparent color is index 2

  EXPECT_TRUE(!doc->sprite()->root()->firstLayer()->isBackground());
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 2);
  put_pixel(src, 1, 1, 1);

  base::UniquePtr<Image> dst(Image::create(IMAGE_INDEXED, 2, 2));
  clear_image(dst, 1);
  EXPECT_2X2_PIXELS(dst, 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_2X2_PIXELS(dst, 2, 2, 2, 1); // Indexed transparent

  dst.reset(Image::create(IMAGE_RGB, 2, 2));
  clear_image(dst, 1);
  EXPECT_2X2_PIXELS(dst, 1, 1, 1, 1);
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  color_t c1 = doc->sprite()->palette(0)->entry(1);
  EXPECT_NE(0, c1);
  EXPECT_2X2_PIXELS(dst, 0, 0, 0, c1); // RGB transparent
}

TEST(Render, CheckedBackground)
{
  Context ctx;
  Document* doc = ctx.documents().add(4, 4, ColorMode::RGB);

  base::UniquePtr<Image> dst(Image::create(IMAGE_RGB, 4, 4));
  clear_image(dst, 0);

  Render render;
  render.setBgType(BgType::CHECKED);
  render.setBgZoom(true);
  render.setBgColor1(1);
  render.setBgColor2(2);

  render.setBgCheckedSize(gfx::Size(1, 1));
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst,
    1, 2, 1, 2,
    2, 1, 2, 1,
    1, 2, 1, 2,
    2, 1, 2, 1);

  render.setBgCheckedSize(gfx::Size(2, 2));
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst,
    1, 1, 2, 2,
    1, 1, 2, 2,
    2, 2, 1, 1,
    2, 2, 1, 1);

  render.setBgCheckedSize(gfx::Size(3, 3));
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst,
    1, 1, 1, 2,
    1, 1, 1, 2,
    1, 1, 1, 2,
    2, 2, 2, 1);

  render.setProjection(Projection(PixelRatio(1, 1), Zoom(2, 1)));
  render.setBgCheckedSize(gfx::Size(1, 1));
  render.renderSprite(dst, doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst,
    1, 1, 2, 2,
    1, 1, 2, 2,
    2, 2, 1, 1,
    2, 2, 1, 1);
}

TEST(Render, ZoomAndDstBounds)
{
  Context ctx;

  // Create this image:
  // 0 0 0
  // 0 4 4
  // 0 4 4
  Document* doc = ctx.documents().add(3, 3, ColorMode::RGB);
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 0);
  fill_rect(src, 1, 1, 2, 2, 4);

  base::UniquePtr<Image> dst(Image::create(IMAGE_RGB, 4, 4));
  clear_image(dst, 0);

  Render render;
  render.setBgType(BgType::CHECKED);
  render.setBgZoom(true);
  render.setBgColor1(1);
  render.setBgColor2(2);
  render.setBgCheckedSize(gfx::Size(1, 1));

  render.renderSprite(
    dst, doc->sprite(), frame_t(0),
    gfx::Clip(1, 1, 0, 0, 2, 2));
  EXPECT_4X4_PIXELS(dst,
    0, 0, 0, 0,
    0, 1, 2, 0,
    0, 2, 4, 0,
    0, 0, 0, 0);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
