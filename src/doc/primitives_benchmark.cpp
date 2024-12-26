// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/primitives.h"

#include "doc/algorithm/random_image.h"
#include "doc/image_ref.h"

#include <benchmark/benchmark.h>

using namespace doc;

void BM_IsSameImageOld(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef a(Image::create(pf, w, h));
  doc::algorithm::random_image(a.get());
  ImageRef b(Image::createCopy(a.get()));
  while (state.KeepRunning()) {
    is_same_image_slow(a.get(), b.get());
  }
}

void BM_IsSameImageNew(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef a(Image::create(pf, w, h));
  doc::algorithm::random_image(a.get());
  ImageRef b(Image::createCopy(a.get()));
  while (state.KeepRunning()) {
    is_same_image(a.get(), b.get());
  }
}

#define DEFARGS()                                                                                  \
  ->Args({ IMAGE_RGB, 16, 16 })                                                                    \
    ->Args({ IMAGE_RGB, 1024, 1024 })                                                              \
    ->Args({ IMAGE_RGB, 8192, 8192 })                                                              \
    ->Args({ IMAGE_GRAYSCALE, 16, 16 })                                                            \
    ->Args({ IMAGE_GRAYSCALE, 1024, 1024 })                                                        \
    ->Args({ IMAGE_GRAYSCALE, 8192, 8192 })                                                        \
    ->Args({ IMAGE_INDEXED, 16, 16 })                                                              \
    ->Args({ IMAGE_INDEXED, 1024, 1024 })                                                          \
    ->Args({ IMAGE_INDEXED, 8192, 8192 })

BENCHMARK(BM_IsSameImageOld)
DEFARGS()->UseRealTime();

BENCHMARK(BM_IsSameImageNew)
DEFARGS()->UseRealTime();

BENCHMARK_MAIN();
