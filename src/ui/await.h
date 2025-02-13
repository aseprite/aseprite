// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_AWAIT_H_INCLUDED
#define UI_AWAIT_H_INCLUDED
#include <type_traits>
#pragma once

#include "os/event.h"
#include "os/event_queue.h"
#include "ui/system.h"

#include <functional>
#include <future>

namespace ui {

// Awaits for the future result of the function passed.
// It is meant to be used from background threads to ask something to the main
// thread and wait for its result.
// If await is called from the main thread it just executes the function
// and returns the result.
template<typename T>
T await(std::function<T()>&& func)
{
  if (ui::is_ui_thread())
    return func();

  std::promise<T> promise;
  std::future<T> future = promise.get_future();

  // Queue the event
  os::Event ev;
  ev.setType(os::Event::Callback);
  ev.setCallback([func, &promise]() {
    try {
      if constexpr (std::is_void<T>())
        func();
      else
        promise.set_value(func());
    }
    catch (...) {
      promise.set_exception(std::current_exception());
    }
  });
  os::queue_event(ev);
  // Wait for the future
  return future.get();
}

} // namespace ui

#endif
