// Aseprite UI Library
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_OVERLAY_H_INCLUDED
#define UI_OVERLAY_H_INCLUDED
#pragma once

#include "base/ref.h"
#include "gfx/fwd.h"
#include "gfx/point.h"
#include "os/surface.h"
#include "ui/base.h"

namespace os {
class Surface;
}

namespace ui {
class Display;

class Overlay;
using OverlayRef = base::Ref<Overlay>;

class Overlay : public base::RefCountT<Overlay> {
public:
  enum ZOrder { NormalZOrder = 0, MouseZOrder = 5000 };

  Overlay(Display* display,
          const os::SurfaceRef& overlaySurface,
          const gfx::Point& pos,
          ZOrder zorder = NormalZOrder);
  ~Overlay();

  os::SurfaceRef setSurface(const os::SurfaceRef& newSurface);

  const gfx::Point& position() const { return m_pos; }
  gfx::Rect bounds() const;

  void captureOverlappedArea();
  void restoreOverlappedArea(const gfx::Rect& restoreBounds);

  void drawOverlay();
  void moveOverlay(const gfx::Point& newPos);

  bool operator<(const Overlay& other) const { return m_zorder < other.m_zorder; }

private:
  Display* m_display;
  os::SurfaceRef m_surface;
  os::SurfaceRef m_overlap;

  // Surface where we captured the overlapped (m_overlap)
  // region. It's nullptr if the overlay wasn't drawn yet.
  os::SurfaceRef m_captured;

  gfx::Point m_pos;
  ZOrder m_zorder;
};

} // namespace ui

#endif
