// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_display.h"

#ifdef _WIN32
  #include "she/win/event_queue.h"
#else
  #error Your platform does not have a EventQueue implementation
#endif

namespace she {

SkiaDisplay::SkiaDisplay(int width, int height, int scale)
  :
#ifdef _WIN32
  m_queue(new WinEventQueue)
#endif
  , m_window(m_queue, this)
  , m_surface(new SkiaSurface)
{
  m_surface->create(width, height);
  m_window.setScale(scale);
  m_window.setVisible(true);
  m_recreated = false;
}

void SkiaDisplay::resize(const gfx::Size& size)
{
  m_surface->dispose();
  m_surface = new SkiaSurface;
  m_surface->create(size.w, size.h);
  m_recreated = true;
}

void SkiaDisplay::dispose()
{
  delete this;
}

int SkiaDisplay::width() const
{
  return m_window.clientSize().w;
}

int SkiaDisplay::height() const
{
  return m_window.clientSize().h;
}

int SkiaDisplay::originalWidth() const
{
  return m_window.restoredSize().w;
}

int SkiaDisplay::originalHeight() const
{
  return m_window.restoredSize().h;
}

void SkiaDisplay::setScale(int scale)
{
  m_window.setScale(scale);
}

int SkiaDisplay::scale() const
{
  return m_window.scale();
}

NonDisposableSurface* SkiaDisplay::getSurface()
{
  return static_cast<NonDisposableSurface*>(m_surface);
}

// Flips all graphics in the surface to the real display.  Returns
// false if the flip couldn't be done because the display was
// resized.
bool SkiaDisplay::flip()
{
  if (m_recreated) {
    m_recreated = false;
    return false;
  }

  m_window.invalidate();
  return true;
}

void SkiaDisplay::maximize()
{
  m_window.maximize();
}

bool SkiaDisplay::isMaximized() const
{
  return m_window.isMaximized();
}

void SkiaDisplay::setTitleBar(const std::string& title)
{
  m_window.setText(title);
}

EventQueue* SkiaDisplay::getEventQueue()
{
  return m_queue;
}

bool SkiaDisplay::setNativeMouseCursor(NativeCursor cursor)
{
  m_window.setNativeMouseCursor(cursor);
  return true;
}

void SkiaDisplay::setMousePosition(const gfx::Point& position)
{
  m_window.setMousePosition(position);
}

void SkiaDisplay::captureMouse()
{
  m_window.captureMouse();
}

void SkiaDisplay::releaseMouse()
{
  m_window.releaseMouse();
}

DisplayHandle SkiaDisplay::nativeHandle()
{
  return (DisplayHandle)m_window.handle();
}

} // namespace she
