// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/overlay.h"

#include "she/locked_surface.h"
#include "she/scoped_surface_lock.h"
#include "she/system.h"
#include "ui/manager.h"

namespace ui {

Overlay::Overlay(she::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder)
  : m_surface(overlaySurface)
  , m_overlap(NULL)
  , m_pos(pos)
  , m_zorder(zorder)
{
}

Overlay::~Overlay()
{
  Manager* manager = Manager::getDefault();
  if (manager)
    manager->invalidateRect(gfx::Rect(m_pos.x, m_pos.y,
                                      m_surface->width(),
                                      m_surface->height()));
  m_surface->dispose();
  if (m_overlap)
    m_overlap->dispose();
}

gfx::Rect Overlay::getBounds() const
{
  return gfx::Rect(m_pos.x, m_pos.y, m_surface->width(), m_surface->height());
}

void Overlay::drawOverlay(she::LockedSurface* screen)
{
  she::ScopedSurfaceLock lockedSurface(m_surface);
  screen->drawAlphaSurface(lockedSurface, m_pos.x, m_pos.y);
}

void Overlay::moveOverlay(const gfx::Point& newPos)
{
  m_pos = newPos;
}

void Overlay::captureOverlappedArea(she::LockedSurface* screen)
{
  if (!m_overlap)
    m_overlap = she::Instance()->createSurface(m_surface->width(), m_surface->height());

  she::ScopedSurfaceLock lock(m_overlap);
  screen->blitTo(lock, m_pos.x, m_pos.y, 0, 0,
                 m_overlap->width(), m_overlap->height());
}

void Overlay::restoreOverlappedArea(she::LockedSurface* screen)
{
  if (!m_overlap)
    return;

  she::ScopedSurfaceLock lock(m_overlap);
  lock->blitTo(screen, 0, 0, m_pos.x, m_pos.y,
               m_overlap->width(), m_overlap->height());
}

}
