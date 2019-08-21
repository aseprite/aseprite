// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/shrink_bounds.h"

#include "doc/color.h"
#include "doc/image.h"

#include <benchmark/benchmark.h>
#include <memory>

using namespace doc;

void BM_ShrinkBounds(benchmark::State& state) {
  const PixelFormat pixelFormat = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);

  std::unique_ptr<Image> img(Image::create(pixelFormat, w, h));
  img->putPixel(w/2, h/2, rgba(1, 2, 3, 4));
  gfx::Rect rc;
  while (state.KeepRunning()) {
    doc::algorithm::shrink_bounds(img.get(), rc, 0);
  }
}

#define DEFARGS(MODE)                      \
  ->Args({ MODE, 100, 100 })               \
  ->Args({ MODE, 200, 200 })               \
  ->Args({ MODE, 300, 300 })               \
  ->Args({ MODE, 400, 400 })               \
  ->Args({ MODE, 499, 500 })               \
  ->Args({ MODE, 500, 500 })               \
  ->Args({ MODE, 600, 600 })               \
  ->Args({ MODE, 700, 700 })               \
  ->Args({ MODE, 799, 800 })               \
  ->Args({ MODE, 800, 800 })               \
  ->Args({ MODE, 900, 900 })               \
  ->Args({ MODE, 1000, 1000 })             \
  ->Args({ MODE, 1500, 1500 })             \
  ->Args({ MODE, 2000, 2000 })             \
  ->Args({ MODE, 4000, 4000 })             \
  ->Args({ MODE, 8000, 8000 })

BENCHMARK(BM_ShrinkBounds)
  DEFARGS(IMAGE_RGB)
  DEFARGS(IMAGE_GRAYSCALE)
  DEFARGS(IMAGE_INDEXED)
  ->Unit(benchmark::kMicrosecond)
  ->UseRealTime();

BENCHMARK_MAIN();
