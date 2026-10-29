// Aseprite Document Library
// Copyright (c) 2024-present Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/blend_funcs.h"

#include <benchmark/benchmark.h>

using namespace doc;

static void CustomArguments(benchmark::internal::Benchmark* b)
{
  b->Args({ int(rgba(200, 128, 64, 255)), int(rgba(32, 128, 200, 255)), 255 })
    ->Args({ int(rgba(200, 128, 64, 255)), int(rgba(32, 128, 200, 255)), 128 })
    ->Args({ int(rgba(200, 128, 64, 255)), int(rgba(32, 128, 200, 255)), 0 })
    ->Args({ int(rgba(200, 128, 64, 128)), int(rgba(32, 128, 200, 128)), 255 })
    ->Args({ int(rgba(200, 128, 64, 128)), int(rgba(32, 128, 200, 128)), 128 })
    ->Args({ int(rgba(200, 128, 64, 128)), int(rgba(32, 128, 200, 128)), 0 })
    ->Args({ int(rgba(200, 128, 64, 128)), int(rgba(32, 128, 200, 0)), 255 });
}

#define RGBA_BENCHMARK(Mode)                                                                       \
  void BM_Rgba_##Mode(benchmark::State& state)                                                     \
  {                                                                                                \
    color_t a = color_t(state.range(0));                                                           \
    color_t b = color_t(state.range(1));                                                           \
    int opacity = state.range(2);                                                                  \
    BlendFunc func = get_rgba_blender(BlendMode::Mode);                                            \
    for (auto _ : state) {                                                                         \
      color_t c = func(a, b, opacity);                                                             \
      benchmark::DoNotOptimize(c);                                                                 \
    }                                                                                              \
  }                                                                                                \
  BENCHMARK(BM_Rgba_##Mode)->Apply(CustomArguments)

RGBA_BENCHMARK(NORMAL);
RGBA_BENCHMARK(MULTIPLY);
RGBA_BENCHMARK(SCREEN);
RGBA_BENCHMARK(OVERLAY);
RGBA_BENCHMARK(DARKEN);
RGBA_BENCHMARK(LIGHTEN);
RGBA_BENCHMARK(COLOR_DODGE);
RGBA_BENCHMARK(COLOR_BURN);
RGBA_BENCHMARK(HARD_LIGHT);
RGBA_BENCHMARK(SOFT_LIGHT);
RGBA_BENCHMARK(DIFFERENCE);
RGBA_BENCHMARK(EXCLUSION);
RGBA_BENCHMARK(HSL_HUE);
RGBA_BENCHMARK(HSL_SATURATION);
RGBA_BENCHMARK(HSL_COLOR);
RGBA_BENCHMARK(HSL_LUMINOSITY);
RGBA_BENCHMARK(ADDITION);
RGBA_BENCHMARK(SUBTRACT);
RGBA_BENCHMARK(DIVIDE);

BENCHMARK_MAIN();
