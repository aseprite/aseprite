// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_INCLUDED
#pragma once

#ifdef _WIN32
  #include "she/win/window.h"
#else
  #error There is no Window implementation
#endif

#include "she/skia/skia_event_queue.h"
#include "she/skia/skia_surface.h"

namespace she {

class SkiaDisplay;

class SkiaWindow : public Window<SkiaWindow> {
public:
  SkiaWindow(SkiaEventQueue* queue, SkiaDisplay* display);
  void queueEventImpl(Event& ev);
  void paintImpl(HDC hdc);
  void resizeImpl(const gfx::Size& size);

private:
  SkiaEventQueue* m_queue;
  SkiaDisplay* m_display;
};

} // namespace she

#endif
