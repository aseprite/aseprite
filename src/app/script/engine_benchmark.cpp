// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include <benchmark/benchmark.h>

#ifdef ENABLE_SCRIPTING
  #include "engine.h"

using namespace app;

static void BM_EngineInit(benchmark::State& state)
{
  for (auto _ : state) {
    script::Engine engine;
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_EngineInit)->Unit(benchmark::kMicrosecond);

static void BM_EngineBasicEval(benchmark::State& state)
{
  for (auto _ : state) {
    script::Engine engine;
    engine.evalCode("app.pixelColor.rgba(255, 255, 255)");
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_EngineBasicEval)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
#endif
