// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/doc.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui_context.h"
#include "doc/sprite.h"
#include "os/system.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/timer.h"

#include <benchmark/benchmark.h>

using namespace app;
using namespace doc;

void BM_ScrollEditor(benchmark::State& state)
{
  const int w = state.range(0);
  const int h = state.range(1);
  const int zNum = state.range(2);
  const int zDen = state.range(3);
  auto ctx = UIContext::instance();

  Sprite* spr = new Sprite(ImageSpec(ColorMode::RGB, w, h), 256);
  std::unique_ptr<Doc> doc(new Doc(spr));
  doc->setContext(ctx);

  Editor* editor = ctx->activeEditor();
  editor->setZoom(render::Zoom(zNum, zDen));

  auto mgr = ui::Manager::getDefault();

  ui::Timer timer(1);
  timer.start();
  while (state.KeepRunning()) {
    editor->setEditorScroll(gfx::Point(0, 0));
    mgr->generateMessages();
    mgr->dispatchMessages();

    editor->setEditorScroll(gfx::Point(100, 100));
    mgr->generateMessages();
    mgr->dispatchMessages();
  }
}

void BM_ZoomEditor(benchmark::State& state)
{
  const int w = state.range(0);
  const int h = state.range(1);
  auto ctx = UIContext::instance();

  Sprite* spr = new Sprite(ImageSpec(ColorMode::RGB, w, h), 256);
  std::unique_ptr<Doc> doc(new Doc(spr));
  doc->setContext(ctx);

  Editor* editor = ctx->activeEditor();
  editor->setZoom(render::Zoom(1, 1));
  editor->setScrollToCenter();

  auto mgr = ui::Manager::getDefault();

  ui::Timer timer(1);
  timer.start();
  while (state.KeepRunning()) {
    editor->setZoom(render::Zoom(4, 1));
    editor->invalidate();
    mgr->generateMessages();
    mgr->dispatchMessages();

    editor->setZoom(render::Zoom(2, 1));
    editor->invalidate();
    mgr->generateMessages();
    mgr->dispatchMessages();

    editor->setZoom(render::Zoom(1, 1));
    editor->invalidate();
    mgr->generateMessages();
    mgr->dispatchMessages();

    editor->setZoom(render::Zoom(1, 2));
    editor->invalidate();
    mgr->generateMessages();
    mgr->dispatchMessages();

    editor->setZoom(render::Zoom(1, 4));
    editor->invalidate();
    mgr->generateMessages();
    mgr->dispatchMessages();
  }
}

BENCHMARK(BM_ScrollEditor)
  // Normal zoom
  ->Args({ 32, 32, 1, 1 })
  ->Args({ 256, 256, 1, 1 })
  ->Args({ 1024, 1024, 1, 1 })
  ->Args({ 4096, 4096, 1, 1 })
  // Zoom in
  ->Args({ 32, 32, 2, 1 })
  ->Args({ 256, 256, 2, 1 })
  ->Args({ 1024, 1024, 2, 1 })
  ->Args({ 4096, 4096, 2, 1 })
  ->Args({ 32, 32, 4, 1 })
  ->Args({ 256, 256, 4, 1 })
  ->Args({ 1024, 1024, 4, 1 })
  ->Args({ 4096, 4096, 4, 1 })
  // Zoom out
  ->Args({ 32, 32, 1, 2 })
  ->Args({ 256, 256, 1, 2 })
  ->Args({ 1024, 1024, 1, 2 })
  ->Args({ 4096, 4096, 1, 2 })
  ->Args({ 32, 32, 1, 4 })
  ->Args({ 256, 256, 1, 4 })
  ->Args({ 1024, 1024, 1, 4 })
  ->Args({ 4096, 4096, 1, 4 })
  ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ZoomEditor)
  ->Args({ 32, 32 })
  ->Args({ 256, 256 })
  ->Args({ 1024, 1024 })
  ->Args({ 4096, 4096 })
  ->Unit(benchmark::kMicrosecond);

int app_main(int argc, char* argv[])
{
  os::SystemRef system(os::make_system());
  App app;
  const char* argv2[] = { argv[0] };
  app.initialize(AppOptions(1, { argv2 }));
  app.mainWindow()->expandWindow(gfx::Size(400, 300));

  ::benchmark::Initialize(&argc, argv);
  int status = ::benchmark::RunSpecifiedBenchmarks();

  app.close();
  return status;
}
