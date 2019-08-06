// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
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

using namespace doc;
using namespace render;

static void Bm_Render(benchmark::State& state)
{
  const int w = state.range(0);
  const int h = state.range(1);

  Sprite* spr = Sprite::MakeStdSprite(ImageSpec(ColorMode::RGB, w, h));
  LayerImage* lay1 = static_cast<LayerImage*>(spr->root()->firstLayer());
  LayerImage* lay2 = new LayerImage(spr);
  LayerImage* lay3 = new LayerImage(spr);

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
  fill_rect(img1, 32, 32, w-64, h-64, rgba(32, 128, 255, 128));
  fill_rect(img2.get(), 0, 0, w-64, h-64, rgba(255, 100, 32, 128));
  fill_rect(img3.get(), 64, 64, w-64, h-64, rgba(200, 64, 80, 128));

  std::unique_ptr<Image> dst(Image::create(spr->pixelFormat(), w, h));
  clear_image(dst.get(), 0);

  while (state.KeepRunning()) {
    clear_image(dst.get(), 0);

    Render render;
    render.setBgType(BgType::CHECKED);
    render.setBgZoom(true);
    render.setBgColor1(rgba(100, 100, 100, 255));
    render.setBgColor2(rgba(200, 200, 200, 255));
    render.setBgCheckedSize(gfx::Size(16, 16));
    render.renderSprite(
      dst.get(), spr, frame_t(0),
      gfx::Clip(0, 0, 0, 0, w, h));
  }
}

BENCHMARK(Bm_Render)
  ->Args({ 256, 256 })
  ->Args({ 1024, 256 })
  ->Args({ 256, 1024 })
  ->Args({ 1024, 1024 })
  ->Args({ 4096, 4096 })
  ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
