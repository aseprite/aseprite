// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_X11_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_X11_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"
#include "she/native_cursor.h"
#include "she/x11/window.h"

#include <string>

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public X11Window {
public:
  enum class Backend { NONE, GL };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  void setVisible(bool visible);
  void maximize();
  bool isMaximized() const;
  bool isMinimized() const;

  std::string getLayout() { return ""; }
  void setLayout(const std::string& layout) { }

  void setTranslateDeadKeys(bool state) {
    // Do nothing
  }

private:
  void queueEvent(Event& ev) override;
  void paintGC(const gfx::Rect& rc) override;
  void resizeDisplay(const gfx::Size& sz) override;

  EventQueue* m_queue;
  SkiaDisplay* m_display;

  DISABLE_COPYING(SkiaWindow);
};

} // namespace she

#endif
