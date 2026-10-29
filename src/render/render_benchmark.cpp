// Aseprite Document Library
// Copyright (c) 2019-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "render/render.h"

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

#include <benchmark/benchmark.h>

#include <memory>

using namespace doc;
using namespace render;

static std::unique_ptr<Sprite> create_sprite(ColorMode colorMode, int w, int h)
{
  std::unique_ptr<Sprite> spr(Sprite::MakeStdSprite(ImageSpec(colorMode, w, h)));
  LayerImage* lay1 = static_cast<LayerImage*>(spr->root()->firstLayer());
  LayerImage* lay2 = new LayerImage(spr.get());
  LayerImage* lay3 = new LayerImage(spr.get());

  spr->root()->addLayer(lay2);
  spr->root()->addLayer(lay3);

  Image* img1 = lay1->cel(0)->image();
  ImageRef img2(Image::create(spr->pixelFormat(), w, h));
  ImageRef img3(Image::create(spr->pixelFormat(), w, h));
  Cel* cel2 = new Cel(frame_t(0), img2);
  Cel* cel3 = new Cel(frame_t(0), img3);

  lay2->addCel(cel2);
  lay3->addCel(cel3);

  clear_image(img1, 0);
  clear_image(img2.get(), 0);
  clear_image(img3.get(), 0);
  fill_rect(img1, 32, 32, w - 64, h - 64, rgba(32, 128, 255, 128));
  fill_rect(img2.get(), 0, 0, w - 64, h - 64, rgba(255, 100, 32, 128));
  fill_rect(img3.get(), 64, 64, w - 64, h - 64, rgba(200, 64, 80, 128));

  return spr;
}

static void Bm_Render(benchmark::State& state, const Projection& proj)
{
  const ColorMode colorMode = (ColorMode)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);

  std::unique_ptr<Sprite> spr(create_sprite(colorMode, w, h));
  std::unique_ptr<Image> dst(Image::create(spr->pixelFormat(), proj.applyX(w), proj.applyY(h)));
  clear_image(dst.get(), 0);

  for (auto _ : state) {
    clear_image(dst.get(), 0);

    Render render;
    BgOptions bg;
    bg.type = BgType::CHECKERED;
    bg.zoom = true;
    bg.color1 = rgba(100, 100, 100, 255);
    bg.color2 = rgba(200, 200, 200, 255);
    bg.stripeSize = gfx::Size(16, 16);
    render.setBgOptions(bg);
    render.setProjection(proj);
    render.renderSprite(dst.get(), spr.get(), frame_t(0), gfx::Clip(0, 0, 0, 0, w, h));
  }
}

static void Bm_RenderWithoutScale(benchmark::State& state)
{
  Bm_Render(state, Projection());
}

static void Bm_RenderScaleUp(benchmark::State& state)
{
  Bm_Render(state, Projection(PixelRatio(1, 1), Zoom(8, 1)));
}

static void Bm_RenderScaleDown(benchmark::State& state)
{
  Bm_Render(state, Projection(PixelRatio(1, 1), Zoom(1, 8)));
}

#define BENCH_COLOR_MODE(CM)                                                                       \
  BENCHMARK(Bm_RenderWithoutScale)                                                                 \
    ->Args({ (int)ColorMode::CM, 32, 32 })                                                         \
    ->Args({ (int)ColorMode::CM, 1024, 1024 })                                                     \
    ->Args({ (int)ColorMode::CM, 2048, 2048 })                                                     \
    ->Args({ (int)ColorMode::CM, 32, 2048 })                                                       \
    ->Args({ (int)ColorMode::CM, 2048, 32 })                                                       \
    ->Unit(benchmark::kMicrosecond);                                                               \
                                                                                                   \
  BENCHMARK(Bm_RenderScaleUp)                                                                      \
    ->Args({ (int)ColorMode::CM, 32, 32 })                                                         \
    ->Args({ (int)ColorMode::CM, 1024, 1024 })                                                     \
    ->Args({ (int)ColorMode::CM, 2048, 2048 })                                                     \
    ->Args({ (int)ColorMode::CM, 32, 2048 })                                                       \
    ->Args({ (int)ColorMode::CM, 2048, 32 })                                                       \
    ->Unit(benchmark::kMicrosecond);                                                               \
                                                                                                   \
  BENCHMARK(Bm_RenderScaleDown)                                                                    \
    ->Args({ (int)ColorMode::CM, 32, 32 })                                                         \
    ->Args({ (int)ColorMode::CM, 1024, 1024 })                                                     \
    ->Args({ (int)ColorMode::CM, 2048, 2048 })                                                     \
    ->Args({ (int)ColorMode::CM, 32, 2048 })                                                       \
    ->Args({ (int)ColorMode::CM, 2048, 32 })                                                       \
    ->Unit(benchmark::kMicrosecond);

BENCH_COLOR_MODE(RGB)
BENCH_COLOR_MODE(GRAYSCALE)
BENCH_COLOR_MODE(INDEXED)

BENCHMARK_MAIN();
