// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/flip_image.h"

#include "doc/image.h"

#include <benchmark/benchmark.h>

using namespace doc;

void BM_FlipSlow(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  const auto ft = (doc::algorithm::FlipType)state.range(3);
  std::unique_ptr<Image> img(Image::create(pf, w, h));
  while (state.KeepRunning()) {
    algorithm::flip_image_slow(img.get(), img->bounds(), ft);
  }
}

void BM_FlipRawPtr(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  const auto ft = (doc::algorithm::FlipType)state.range(3);
  std::unique_ptr<Image> img(Image::create(pf, w, h));
  while (state.KeepRunning()) {
    algorithm::flip_image(img.get(), img->bounds(), ft);
  }
}

#define DEFARGS()                                                                                  \
  ->Args({ IMAGE_RGB, 8192, 8192, doc::algorithm::FlipHorizontal })                                \
    ->Args({ IMAGE_RGB, 8192, 8192, doc::algorithm::FlipVertical })                                \
    ->Args({ IMAGE_GRAYSCALE, 8192, 8192, doc::algorithm::FlipHorizontal })                        \
    ->Args({ IMAGE_GRAYSCALE, 8192, 8192, doc::algorithm::FlipVertical })                          \
    ->Args({ IMAGE_INDEXED, 8192, 8192, doc::algorithm::FlipHorizontal })                          \
    ->Args({ IMAGE_INDEXED, 8192, 8192, doc::algorithm::FlipVertical })                            \
    ->Args({ IMAGE_BITMAP, 8192, 8192, doc::algorithm::FlipHorizontal })                           \
    ->Args({ IMAGE_BITMAP, 8192, 8192, doc::algorithm::FlipVertical })                             \
    ->Args({ IMAGE_TILEMAP, 8192, 8192, doc::algorithm::FlipHorizontal })                          \
    ->Args({ IMAGE_TILEMAP, 8192, 8192, doc::algorithm::FlipVertical })

BENCHMARK(BM_FlipSlow)
DEFARGS()->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK(BM_FlipRawPtr)
DEFARGS()->Unit(benchmark::kMicrosecond)->UseRealTime();

BENCHMARK_MAIN();
