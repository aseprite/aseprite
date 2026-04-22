// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/random_image.h"
#include "doc/image.h"
#include "doc/image_ref.h"

#include <benchmark/benchmark.h>

using namespace doc;

void BM_ForXYGetPixel(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef img(Image::create(pf, w, h));
  doc::algorithm::random_image(img.get());
  for (auto _ : state) {
    color_t c = 0;
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        c += img->getPixel(x, y);
      }
    }
    benchmark::DoNotOptimize(c);
  }
}

void BM_ForEachPixel(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef img(Image::create(pf, w, h));
  doc::algorithm::random_image(img.get());
  for (auto _ : state) {
    color_t c = 0;
    auto func = [&c](color_t u) { c += u; };
    switch (pf) {
      case IMAGE_RGB:       for_each_pixel<RgbTraits>(img.get(), func); break;
      case IMAGE_GRAYSCALE: for_each_pixel<GrayscaleTraits>(img.get(), func); break;
      case IMAGE_INDEXED:   for_each_pixel<IndexedTraits>(img.get(), func); break;
      case IMAGE_BITMAP:    for_each_pixel<BitmapTraits>(img.get(), func); break;
      case IMAGE_TILEMAP:   for_each_pixel<TilemapTraits>(img.get(), func); break;
    }
    benchmark::DoNotOptimize(c);
  }
}

void BM_ForXYGetPixelFast(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef img(Image::create(pf, w, h));
  doc::algorithm::random_image(img.get());
  for (auto _ : state) {
    color_t c = 0;
    switch (pf) {
      case IMAGE_RGB:
        for (int y = 0; y < h; ++y) {
          for (int x = 0; x < w; ++x) {
            c += get_pixel_fast<RgbTraits>(img.get(), x, y);
          }
        }
        break;
      case IMAGE_GRAYSCALE:
        for (int y = 0; y < h; ++y) {
          for (int x = 0; x < w; ++x) {
            c += get_pixel_fast<GrayscaleTraits>(img.get(), x, y);
          }
        }
        break;
      case IMAGE_INDEXED:
        for (int y = 0; y < h; ++y) {
          for (int x = 0; x < w; ++x) {
            c += get_pixel_fast<IndexedTraits>(img.get(), x, y);
          }
        }
        break;
      case IMAGE_BITMAP:
        for (int y = 0; y < h; ++y) {
          for (int x = 0; x < w; ++x) {
            c += get_pixel_fast<BitmapTraits>(img.get(), x, y);
          }
        }
        break;
    }
    benchmark::DoNotOptimize(c);
  }
}

template<typename Func>
void dispatch_by_pixel_format(Image* image, const ReadIterator& it, Func func)
{
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       func(it.addr32()); break;
    case IMAGE_GRAYSCALE: func(it.addr16()); break;
    case IMAGE_INDEXED:   func(it.addr8()); break;
#if !DOC_USE_BITMAP_AS_1BPP
    case IMAGE_BITMAP: func(it.addr8()); break;
#endif
    case IMAGE_TILEMAP: func(it.addr32()); break;
  }
}

void BM_ReadIterator(benchmark::State& state)
{
  const auto pf = (PixelFormat)state.range(0);
  const int w = state.range(1);
  const int h = state.range(2);
  ImageRef img(Image::create(pf, w, h));
  doc::algorithm::random_image(img.get());
  for (auto _ : state) {
    auto it = img->readArea(img->bounds());
    while (it.nextLine()) {
      dispatch_by_pixel_format(img.get(), it, [w](auto addr) {
        color_t c = 0;
        for (int x = 0; x < w; ++x, ++addr) {
          c += *addr;
        }
        benchmark::DoNotOptimize(c);
      });
    }
  }
}

#define DEFARGS()                                                                                  \
  ->Args({ IMAGE_RGB, 1024, 1024 })                                                                \
    ->Args({ IMAGE_GRAYSCALE, 1024, 1024 })                                                        \
    ->Args({ IMAGE_INDEXED, 1024, 1024 })                                                          \
    ->Args({ IMAGE_BITMAP, 1024, 1024 })

#if DOC_USE_BITMAP_AS_1BPP

  // In this case we avoid IMAGE_BITMAP as its iterator goes through
  // bits instead of bytes, i.e. a pixel cannot be addressed with a
  // memory pointer.
  #define DEFARGS_ADDRESSABLE_ONLY()                                                               \
    ->Args({ IMAGE_RGB, 1024, 1024 })                                                              \
      ->Args({ IMAGE_GRAYSCALE, 1024, 1024 })                                                      \
      ->Args({ IMAGE_INDEXED, 1024, 1024 })

#else

  #define DEFARGS_ADDRESSABLE_ONLY DEFARGS

#endif

BENCHMARK(BM_ForXYGetPixel)
DEFARGS()->UseRealTime();

BENCHMARK(BM_ForEachPixel)
DEFARGS()->UseRealTime();

BENCHMARK(BM_ForXYGetPixelFast)
DEFARGS()->UseRealTime();

BENCHMARK(BM_ReadIterator)
DEFARGS_ADDRESSABLE_ONLY()->UseRealTime();

BENCHMARK_MAIN();
