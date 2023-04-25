// Aseprite UI Library
// Copyright (C) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_DISPLAY_H_INCLUDED
#define UI_DISPLAY_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "os/window.h"

#include <vector>

namespace ui {
  class Widget;
  class Window;

  // Wraps a native window (os::Window). On each "display" we can show
  // the main manager or a window (containedWidget), and a set of
  // children window that doesn't have a native window counterpart
  // (tooltips?)
  class Display {
  public:
    Display(Display* parentDisplay,
            const os::WindowRef& nativeWindow,
            Widget* containedWidget);

    Display* parentDisplay() { return m_parentDisplay; }
    os::Window* nativeWindow() const { return m_nativeWindow.get(); }
    os::Surface* surface() const;

    int scale() const { return m_nativeWindow->scale(); }
    gfx::Size size() const;
    gfx::Rect bounds() const { return gfx::Rect(size()); }

    Widget* containedWidget() const { return m_containedWidget; }

    // Mark the given rectangle as a area to be flipped to the real
    // screen.
    void dirtyRect(const gfx::Rect& bounds);

    // Refreshes the real display with the UI content.
    void flipDisplay();

    // Returns the invalid region in the screen to being updated with
    // PaintMessages. This region is cleared when each widget receives
    // a paint message.
    const gfx::Region& getInvalidRegion() const {
      return m_invalidRegion;
    }

    void addInvalidRegion(const gfx::Region& b) {
      m_invalidRegion |= b;
    }

    void subtractInvalidRegion(const gfx::Region& b) {
      m_invalidRegion -= b;
    }

    void setInvalidRegion(const gfx::Region& b) {
      m_invalidRegion = b;
    }

    void invalidateRect(const gfx::Rect& rect);
    void invalidateRegion(const gfx::Region& region);

    void addWindow(Window* window);
    void removeWindow(Window* window);
    void handleWindowZOrder(Window* window);
    const std::vector<Window*>& getWindows() const { return m_windows; }

    gfx::Size workareaSizeUIScale();

    const gfx::Point& lastMousePos() const { return m_lastMousePos; }
    void updateLastMousePos(const gfx::Point& pos) { m_lastMousePos = pos; }

    void _setParentDisplay(Display* parentDisplay) {
      m_parentDisplay = parentDisplay;
    }

  private:
    Display* m_parentDisplay;
    os::WindowRef m_nativeWindow;
    Widget* m_containedWidget;      // A ui::Manager or a ui::Window
    std::vector<Window*> m_windows; // Sub-windows in this display
    gfx::Region m_invalidRegion;    // Invalid region (we didn't receive paint messages yet for this).
    gfx::Region m_dirtyRegion;      // Region to flip to the os::Display
    gfx::Point m_lastMousePos;
  };

} // namespace ui

#endif
