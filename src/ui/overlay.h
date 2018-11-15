// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_OVERLAY_H_INCLUDED
#define UI_OVERLAY_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "gfx/point.h"
#include "ui/base.h"

namespace os {
  class Surface;
}

namespace ui {

  class Overlay {
  public:
    typedef int ZOrder;

    static const ZOrder NormalZOrder = 0;
    static const ZOrder MouseZOrder = 5000;

    Overlay(os::Surface* overlaySurface, const gfx::Point& pos, ZOrder zorder = 0);
    ~Overlay();

    os::Surface* setSurface(os::Surface* newSurface);

    const gfx::Point& position() const { return m_pos; }
    gfx::Rect bounds() const;

    void captureOverlappedArea(os::Surface* screen);
    void restoreOverlappedArea(const gfx::Rect& restoreBounds);

    void drawOverlay();
    void moveOverlay(const gfx::Point& newPos);

    bool operator<(const Overlay& other) const {
      return m_zorder < other.m_zorder;
    }

  private:
    os::Surface* m_surface;
    os::Surface* m_overlap;

    // Surface where we captured the overlapped (m_overlap)
    // region. It's nullptr if the overlay wasn't drawn yet.
    os::Surface* m_captured;

    gfx::Point m_pos;
    ZOrder m_zorder;
  };

} // namespace ui

#endif
