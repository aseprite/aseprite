// Aseprite UI Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "fmt/format.h"
#include "os/system.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/timer.h"
#include "ui/view.h"
#include "ui/window.h"

#include <benchmark/benchmark.h>

ui::View* g_view;

void BM_ViewBase(benchmark::State& state)
{
  auto mgr = ui::Manager::getDefault();
  mgr->layout();
  mgr->dontWaitEvents();

  for (auto _ : state) {
    // Do nothing case
    mgr->generateMessages();
    mgr->dispatchMessages();
  }
}

void BM_ViewScrollListBox(benchmark::State& state)
{
  auto mgr = ui::Manager::getDefault();
  auto* view = g_view;

  ui::ListBox list;
  for (int i = 0; i < 1000; ++i)
    list.addChild(new ui::ListItem(fmt::format("List Item {}", i)));
  view->attachToView(&list);

  mgr->layout();
  mgr->dontWaitEvents();

  const auto dy = state.range(0);
  const auto max = view->getScrollableSize();
  int y = 0;

  for (auto _ : state) {
    view->setViewScroll(gfx::Point(0, y));
    mgr->generateMessages();
    mgr->dispatchMessages();

    y += dy;
    if (y >= max.h)
      y = 0;
  }
}

BENCHMARK(BM_ViewBase)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_ViewScrollListBox)
  ->Args({ 0 })
  ->Args({ 1 })
  ->Args({ 6 })
  ->Args({ 12 })
  ->Args({ 16 })
  ->Args({ 256 })
  ->Unit(benchmark::kMicrosecond);

int app_main(int argc, char* argv[])
{
  os::SystemRef system = os::System::make();
  os::WindowRef nativeWindow = system->makeWindow(800, 600);
  ui::UISystem uiSystem;
  ui::Manager manager(nativeWindow);

  ui::Theme theme;
  ui::set_theme(&theme, 1);

  ui::Window window(ui::Window::DesktopWindow);

  window.addChild(g_view = new ui::View);
  window.openWindow();

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  return 0;
}
