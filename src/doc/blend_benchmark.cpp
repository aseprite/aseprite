// Aseprite Document Library
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

template<BlendFunc F>
void BM_Rgba(benchmark::State& state)
{
  color_t a = color_t(state.range(0));
  color_t b = color_t(state.range(1));
  int opacity = state.range(2);
  BlendFunc func = F;
  while (state.KeepRunning()) {
    color_t c = func(a, b, opacity);
    c = func(c, b, opacity);
  }
}

BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_normal)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_multiply)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_screen)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_overlay)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_darken)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_lighten)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_color_dodge)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_color_burn)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_hard_light)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_soft_light)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_difference)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_exclusion)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_hsl_hue)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_hsl_saturation)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_hsl_color)->Apply(CustomArguments);
BENCHMARK_TEMPLATE(BM_Rgba, rgba_blender_hsl_luminosity)->Apply(CustomArguments);

BENCHMARK_MAIN();
