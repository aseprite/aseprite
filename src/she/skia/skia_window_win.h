// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#pragma once

#include "she/skia/skia_surface.h"
#include "she/win/window.h"

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public Window<SkiaWindow> {
public:
  SkiaWindow(EventQueue* queue, SkiaDisplay* display);

  void queueEventImpl(Event& ev);
  void paintImpl(HDC hdc);
  void resizeImpl(const gfx::Size& size);

private:
  EventQueue* m_queue;
  SkiaDisplay* m_display;
};

} // namespace she

#endif
