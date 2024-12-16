// Aseprite UI Library
// Copyright (C) 2019-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/display.h"

#include "base/debug.h"
#include "ui/system.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <algorithm>

namespace ui {

Display::Display(Display* parentDisplay, const os::WindowRef& nativeWindow, Widget* containedWidget)
  : m_parentDisplay(parentDisplay)
  , m_nativeWindow(nativeWindow)
  , m_containedWidget(containedWidget)
{
#if 0 // When compiling tests all these values can be nullptr
  ASSERT(m_nativeWindow);
  ASSERT(m_containedWidget);
  ASSERT(m_containedWidget->type() == kManagerWidget ||
         m_containedWidget->type() == kWindowWidget);
#endif
  m_dirtyRegion = bounds();
}

os::Surface* Display::surface() const
{
  return m_nativeWindow->surface();
}

gfx::Size Display::size() const
{
  // When running tests this can be nullptr
  if (!m_nativeWindow)
    return gfx::Size(1, 1);

  const int scale = m_nativeWindow->scale();
  ASSERT(scale > 0);
  return gfx::Size(m_nativeWindow->width() / scale, m_nativeWindow->height() / scale);
}

void Display::dirtyRect(const gfx::Rect& bounds)
{
  m_dirtyRegion |= gfx::Region(bounds);
}

void Display::flipDisplay()
{
  if (m_dirtyRegion.isEmpty())
    return;

  // Limit the region to the bounds of the window
  m_dirtyRegion &= gfx::Region(bounds());
  if (m_dirtyRegion.isEmpty())
    return;

  // Invalidate the dirty region in the os::Window only if the window
  // is visible (e.g. if the window is hidden or minimized, we don't
  // need to do this).
  if (m_nativeWindow->isVisible()) {
    m_nativeWindow->invalidateRegion(m_dirtyRegion);
    m_nativeWindow->swapBuffers();
  }
  // If the native window is not minimized/hidden by the user, make it
  // visible (e.g. when we flipDisplay() for the first time).
  else if (!m_nativeWindow->isMinimized()) {
    m_nativeWindow->setVisible(true);
  }

  m_dirtyRegion.clear();
}

void Display::invalidateRect(const gfx::Rect& rect)
{
  m_containedWidget->invalidateRect(rect);
}

void Display::invalidateRegion(const gfx::Region& region)
{
  m_containedWidget->invalidateRegion(region);
}

void Display::addWindow(Window* window)
{
  m_windows.insert(m_windows.begin(), window);
}

void Display::removeWindow(Window* window)
{
  auto it = std::find(m_windows.begin(), m_windows.end(), window);
  ASSERT(it != m_windows.end())
  if (it == m_windows.end())
    return;

  m_windows.erase(it);
}

// TODO code similar to Manager::handleWindowZOrder()
void Display::handleWindowZOrder(Window* window)
{
  removeWindow(window);

  if (window->isOnTop())
    m_windows.insert(m_windows.begin(), window);
  else {
    int pos = (int)m_windows.size();

    for (auto it = m_windows.rbegin(), end = m_windows.rend(); it != end; ++it) {
      if (static_cast<Window*>(*it)->isOnTop())
        break;

      --pos;
    }

    m_windows.insert(m_windows.begin() + pos, window);
  }
}

gfx::Size Display::workareaSizeUIScale()
{
  if (get_multiple_displays()) {
    return nativeWindow()->screen()->workarea().size() / nativeWindow()->scale();
  }
  else {
    return size();
  }
}

} // namespace ui
