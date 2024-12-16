// Aseprite UI Library
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/overlay.h"

#include "os/surface.h"
#include "os/system.h"
#include "ui/display.h"

namespace ui {

Overlay::Overlay(Display* display,
                 const os::SurfaceRef& overlaySurface,
                 const gfx::Point& pos,
                 ZOrder zorder)
  : m_display(display)
  , m_surface(overlaySurface)
  , m_overlap(nullptr)
  , m_captured(nullptr)
  , m_pos(pos)
  , m_zorder(zorder)
{
}

Overlay::~Overlay()
{
  ASSERT(!m_captured);

  if (m_surface) {
    if (m_display)
      m_display->invalidateRect(bounds());
    m_surface.reset();
  }

  if (m_overlap)
    m_overlap.reset();
}

os::SurfaceRef Overlay::setSurface(const os::SurfaceRef& newSurface)
{
  os::SurfaceRef oldSurface = m_surface;
  m_surface = newSurface;
  return oldSurface;
}

gfx::Rect Overlay::bounds() const
{
  if (m_surface)
    return gfx::Rect(m_pos.x, m_pos.y, m_surface->width(), m_surface->height());
  else
    return gfx::Rect(0, 0, 0, 0);
}

void Overlay::drawOverlay()
{
  if (!m_surface || !m_captured)
    return;

  os::SurfaceLock lock(m_surface.get());
  m_captured->drawRgbaSurface(m_surface.get(), m_pos.x, m_pos.y);

  m_display->dirtyRect(gfx::Rect(m_pos.x, m_pos.y, m_surface->width(), m_surface->height()));
}

void Overlay::moveOverlay(const gfx::Point& newPos)
{
  if (m_captured)
    restoreOverlappedArea(gfx::Rect());

  m_pos = newPos;
}

void Overlay::captureOverlappedArea()
{
  if (!m_surface || m_captured)
    return;

  os::Surface* displaySurface = m_display->surface();
  os::SurfaceLock lockDisplaySurface(displaySurface);

  if (!m_overlap) {
    // Use the same color space for the overlay as in the screen
    m_overlap = os::instance()->makeSurface(m_surface->width(),
                                            m_surface->height(),
                                            displaySurface->colorSpace());
  }

  os::SurfaceLock lock(m_overlap.get());
  displaySurface
    ->blitTo(m_overlap.get(), m_pos.x, m_pos.y, 0, 0, m_overlap->width(), m_overlap->height());
  // TODO uncomment and test this when GPU support is added
  // m_overlap->setImmutable();

  m_captured = base::AddRef(displaySurface);
}

void Overlay::restoreOverlappedArea(const gfx::Rect& restoreBounds)
{
  if (!m_surface || !m_overlap || !m_captured)
    return;

  if (!restoreBounds.isEmpty() && !restoreBounds.intersects(bounds()))
    return;

  os::SurfaceLock lock(m_overlap.get());
  m_overlap
    ->blitTo(m_captured.get(), 0, 0, m_pos.x, m_pos.y, m_overlap->width(), m_overlap->height());

  m_display->dirtyRect(bounds());
  m_captured = nullptr;
}

} // namespace ui
