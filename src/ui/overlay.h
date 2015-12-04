// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_OVERLAY_H_INCLUDED
#define UI_OVERLAY_H_INCLUDED
#pragma once

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

    static const ZOrder NormalZOrder = 0;
    static const ZOrder MouseZOrder = 5000;

    Overlay(she::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder = 0);
    ~Overlay();

    she::Surface* setSurface(she::Surface* newSurface);

    const gfx::Point& position() const { return m_pos; }
    gfx::Rect bounds() const;

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
