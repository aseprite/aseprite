// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_OVERLAY_H_INCLUDED
#define UI_OVERLAY_H_INCLUDED

#include "gfx/fwd.h"
#include "gfx/point.h"
#include "ui/base.h"

namespace she {
  class Surface;
  class LockedSurface;
}

namespace ui {

  class Overlay {
  public:
    typedef int ZOrder;

    Overlay(she::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder = 0);
    ~Overlay();

    gfx::Rect getBounds() const;

    void captureOverlappedArea(she::LockedSurface* screen);
    void restoreOverlappedArea(she::LockedSurface* screen);

    void drawOverlay(she::LockedSurface* screen);
    void moveOverlay(const gfx::Point& newPos);

    bool operator<(const Overlay& other) const {
      return m_zorder < other.m_zorder;
    }

  private:
    she::Surface* m_surface;
    she::Surface* m_overlap;
    gfx::Point m_pos;
    ZOrder m_zorder;
  };

} // namespace ui

#endif
