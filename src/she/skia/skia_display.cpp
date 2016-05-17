// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_display.h"

#include "she/event.h"
#include "she/event_queue.h"
#include "she/skia/skia_surface.h"
#include "she/system.h"

namespace she {

SkiaDisplay::SkiaDisplay(int width, int height, int scale)
  : m_initialized(false)
  , m_window(instance()->eventQueue(), this, width, height, scale)
  , m_surface(new SkiaSurface)
  , m_customSurface(false)
  , m_nativeCursor(kArrowCursor)
{
  m_window.setScale(scale);
  m_window.setVisible(true);

  resetSkiaSurface();

  m_initialized = true;
}

void SkiaDisplay::setSkiaSurface(SkiaSurface* surface)
{
  m_surface->dispose();
  m_surface = surface;
  m_customSurface = true;
}

void SkiaDisplay::resetSkiaSurface()
{
  if (m_surface) {
    m_surface->dispose();
    m_surface = nullptr;
  }

  gfx::Size size = m_window.clientSize() / m_window.scale();
  m_surface = new SkiaSurface;
  m_surface->create(size.w, size.h);
  m_customSurface = false;
}

void SkiaDisplay::resize(const gfx::Size& size)
{
  Event ev;
  ev.setType(Event::ResizeDisplay);
  ev.setDisplay(this);
  she::queue_event(ev);

  if (!m_initialized || m_customSurface)
    return;

  m_surface->dispose();
  m_surface = new SkiaSurface;
  m_surface->create(size.w / m_window.scale(),
                    size.h / m_window.scale());
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

int SkiaDisplay::scale() const
{
  return m_window.scale();
}

void SkiaDisplay::setScale(int scale)
{
  m_window.setScale(scale);
}

Surface* SkiaDisplay::getSurface()
{
  return m_surface;
}

// Flips all graphics in the surface to the real display.  Returns
// false if the flip couldn't be done because the display was
// resized.
void SkiaDisplay::flip(const gfx::Rect& bounds)
{
  m_window.updateWindow(bounds);
}

void SkiaDisplay::maximize()
{
  m_window.maximize();
}

bool SkiaDisplay::isMaximized() const
{
  return m_window.isMaximized();
}

bool SkiaDisplay::isMinimized() const
{
  return m_window.isMinimized();
}

void SkiaDisplay::setTitleBar(const std::string& title)
{
  m_window.setTitle(title);
}

NativeCursor SkiaDisplay::nativeMouseCursor()
{
  return m_nativeCursor;
}

bool SkiaDisplay::setNativeMouseCursor(NativeCursor cursor)
{
  m_nativeCursor = cursor;
  return m_window.setNativeMouseCursor(cursor);
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

std::string SkiaDisplay::getLayout()
{
  return m_window.getLayout();
}

void SkiaDisplay::setLayout(const std::string& layout)
{
  m_window.setLayout(layout);
}

DisplayHandle SkiaDisplay::nativeHandle()
{
  return (DisplayHandle)m_window.handle();
}

} // namespace she
