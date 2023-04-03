// Aseprite Document Library
// Copyright (c) 2019-2023 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "render/render.h"

#include "doc/cel.h"
#include "doc/document.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"

#include <memory>

using namespace doc;
using namespace render;

template<typename T>
class RenderAllModes : public testing::Test {
protected:
  RenderAllModes() { }
};

typedef testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits> ImageAllTraits;
TYPED_TEST_SUITE(RenderAllModes, ImageAllTraits);

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
  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ColorMode::INDEXED, 2, 2)));

  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 2);

  std::unique_ptr<Image> dst(Image::create(IMAGE_INDEXED, 2, 2));
  clear_image(dst.get(), 1);
  EXPECT_2X2_PIXELS(dst.get(), 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_2X2_PIXELS(dst.get(), 2, 2, 2, 2);
}

TYPED_TEST(RenderAllModes, CheckDefaultBackgroundMode)
{
  typedef TypeParam ImageTraits;

  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ImageTraits::color_mode, 2, 2)));

  EXPECT_TRUE(!doc->sprite()->root()->firstLayer()->isBackground());
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 0);
  put_pixel(src, 1, 1, 1);

  std::unique_ptr<Image> dst(Image::create(ImageTraits::pixel_format, 2, 2));
  clear_image(dst.get(), 1);
  EXPECT_2X2_PIXELS(dst.get(), 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  // Default background mode is to set all pixels to transparent color
  EXPECT_2X2_PIXELS(dst.get(), 0, 0, 0, 1);
}

TEST(Render, DefaultBackgroundModeWithNonzeroTransparentIndex)
{
  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ColorMode::INDEXED, 2, 2)));
  doc->sprite()->setTransparentColor(2); // Transparent color is index 2

  EXPECT_TRUE(!doc->sprite()->root()->firstLayer()->isBackground());
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 2);
  put_pixel(src, 1, 1, 1);

  std::unique_ptr<Image> dst(Image::create(IMAGE_INDEXED, 2, 2));
  clear_image(dst.get(), 1);
  EXPECT_2X2_PIXELS(dst.get(), 1, 1, 1, 1);

  Render render;
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_2X2_PIXELS(dst.get(), 2, 2, 2, 1); // Indexed transparent

  dst.reset(Image::create(IMAGE_RGB, 2, 2));
  clear_image(dst.get(), 1);
  EXPECT_2X2_PIXELS(dst.get(), 1, 1, 1, 1);
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  color_t c1 = doc->sprite()->palette(0)->entry(1);
  EXPECT_NE(0, c1);
  EXPECT_2X2_PIXELS(dst.get(), 0, 0, 0, c1); // RGB transparent
}

TEST(Render, CheckeredBackground)
{
  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ColorMode::INDEXED, 4, 4)));

  std::unique_ptr<Image> dst(Image::create(IMAGE_INDEXED, 4, 4));
  clear_image(dst.get(), 0);

  Render render;
  BgOptions bg;
  bg.type = BgType::CHECKERED;
  bg.zoom = true;
  bg.colorPixelFormat = IMAGE_INDEXED;
  bg.color1 = 1;
  bg.color2 = 2;

  bg.stripeSize = gfx::Size(1, 1);
  render.setBgOptions(bg);
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst.get(),
    1, 2, 1, 2,
    2, 1, 2, 1,
    1, 2, 1, 2,
    2, 1, 2, 1);

  bg.stripeSize = gfx::Size(2, 2);
  render.setBgOptions(bg);
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst.get(),
    1, 1, 2, 2,
    1, 1, 2, 2,
    2, 2, 1, 1,
    2, 2, 1, 1);

  bg.stripeSize = gfx::Size(3, 3);
  render.setBgOptions(bg);
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst.get(),
    1, 1, 1, 2,
    1, 1, 1, 2,
    1, 1, 1, 2,
    2, 2, 2, 1);

  bg.stripeSize = gfx::Size(1, 1);
  render.setProjection(Projection(PixelRatio(1, 1), Zoom(2, 1)));
  render.setBgOptions(bg);
  render.renderSprite(dst.get(), doc->sprite(), frame_t(0));
  EXPECT_4X4_PIXELS(dst.get(),
    1, 1, 2, 2,
    1, 1, 2, 2,
    2, 2, 1, 1,
    2, 2, 1, 1);
}

TEST(Render, ZoomAndDstBounds)
{
  // Create this image:
  // 0 0 0
  // 0 4 4
  // 0 4 4
  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ColorMode::INDEXED, 3, 3)));
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 0);
  fill_rect(src, 1, 1, 2, 2, 4);

  std::unique_ptr<Image> dst(Image::create(IMAGE_INDEXED, 4, 4));
  clear_image(dst.get(), 0);

  Render render;
  BgOptions bg;
  bg.type = BgType::CHECKERED;
  bg.zoom = true;
  bg.colorPixelFormat = IMAGE_INDEXED;
  bg.color1 = 1;
  bg.color2 = 2;
  bg.stripeSize = gfx::Size(1, 1);
  render.setBgOptions(bg);

  render.renderSprite(
    dst.get(), doc->sprite(), frame_t(0),
    gfx::Clip(1, 1, 0, 0, 2, 2));
  EXPECT_4X4_PIXELS(dst.get(),
    0, 0, 0, 0,
    0, 1, 2, 0,
    0, 2, 4, 0,
    0, 0, 0, 0);
}

TEST(Render, BugWithMultiplesOf3ZoomFactors)
{
  std::shared_ptr<Document> doc = std::make_shared<Document>();
  doc->sprites().add(Sprite::MakeStdSprite(ImageSpec(ColorMode::RGB, 4, 4)));
  Image* src = doc->sprite()->root()->firstLayer()->cel(0)->image();
  clear_image(src, 0);
  draw_line(src, 0, 0, 3, 3, rgba(255, 0, 0, 255));

  // Added other factors (like 1, 2, 4, etc.) too
  int zooms[] = { 1, 2, 3, 4, 6, 8, 9, 12, 15, 16, 18, 21, 24, 27,
                  30, 32, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60,
                  63, 66, 69, 72, 75, 78, 81 };
  for (int zoom : zooms) {
    std::unique_ptr<Image> dst(Image::create(IMAGE_RGB, 4*zoom, 4*zoom));
    clear_image(dst.get(), 0);

    Render render;
    BgOptions bg;
    bg.type = BgType::CHECKERED;
    bg.zoom = false;
    bg.colorPixelFormat = IMAGE_RGB;
    bg.color1 = rgba(128, 128, 128, 255);
    bg.color2 = rgba(64, 64, 64, 255);
    bg.stripeSize = gfx::Size(2, 2);
    render.setBgOptions(bg);
    render.setProjection(Projection(PixelRatio(1, 1), Zoom(zoom, 1)));
    render.renderSprite(
      dst.get(), doc->sprite(), frame_t(0),
      gfx::Clip(0, 0, 0, 0, 4*zoom, 4*zoom));

    for (int y=0; y<dst->height(); ++y) {
      for (int x=0; x<dst->width(); ++x) {
        color_t c = get_pixel(dst.get(), x, y);

        if (x / zoom == y / zoom) {
          EXPECT_EQ(c, rgba(255, 0, 0, 255))
            << " zoom=" << zoom << " x=" << x << " y=" << y;
        }
        else {
          EXPECT_NE(c, rgba(255, 0, 0, 255))
            << " zoom=" << zoom << " x=" << x << " y=" << y;

          int gridBg = ((x / 2) + (y / 2)) % 2;
          if (gridBg == 0) {
            EXPECT_EQ(c, rgba(128, 128, 128, 255))
              << " zoom=" << zoom << " x=" << x << " y=" << y;
          }
          else {
            EXPECT_EQ(c, rgba(64, 64, 64, 255))
              << " zoom=" << zoom << " x=" << x << " y=" << y;
          }
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
