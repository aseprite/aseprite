// Aseprite UI Library
// Copyright (C) 2019-2021  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/display.h"

#include "base/debug.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <algorithm>

namespace ui {

Display::Display(Display* parentDisplay,
                 const os::WindowRef& nativeWindow,
                 Widget* containedWidget)
  : m_parentDisplay(parentDisplay)
  , m_nativeWindow(nativeWindow)
  , m_containedWidget(containedWidget)
{
  ASSERT(m_nativeWindow);
  ASSERT(m_containedWidget);
  ASSERT(m_containedWidget->type() == kManagerWidget ||
         m_containedWidget->type() == kWindowWidget);
  m_dirtyRegion = bounds();
}

os::Surface* Display::surface() const
{
  return m_nativeWindow->surface();
}

gfx::Size Display::size() const
{
  ASSERT(m_nativeWindow);
  const int scale = m_nativeWindow->scale();
  ASSERT(scale > 0);
  return gfx::Size(m_nativeWindow->width() / scale,
                   m_nativeWindow->height() / scale);
}

void Display::dirtyRect(const gfx::Rect& bounds)
{
  m_dirtyRegion |= gfx::Region(bounds);
}

void Display::flipDisplay()
{
  if (!m_dirtyRegion.isEmpty()) {
    // Limit the region to the bounds of the window
    m_dirtyRegion &= gfx::Region(bounds());

    if (!m_dirtyRegion.isEmpty()) {
      // Invalidate the dirty region in the os::Window
      m_nativeWindow->invalidateRegion(m_dirtyRegion);
      m_dirtyRegion.clear();
    }
  }
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

    for (auto it=m_windows.rbegin(),
           end=m_windows.rend();
         it != end; ++it) {
      if (static_cast<Window*>(*it)->isOnTop())
        break;

      --pos;
    }

    m_windows.insert(m_windows.begin()+pos, window);
  }
}

} // namespace ui
