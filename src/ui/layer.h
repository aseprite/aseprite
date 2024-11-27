// Aseprite UI Library
// Copyright (C) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LAYER_H_INCLUDED
#define UI_LAYER_H_INCLUDED
#pragma once

#include "base/ref.h"
#include "gfx/region.h"
#include "os/paint.h"
#include "os/surface.h"

namespace os {
  class Surface;
}

namespace ui {

  class UILayer;
  using UILayerRef = base::Ref<UILayer>;
  using UILayers = std::vector<UILayerRef>;

  class UILayer : public base::RefCountT<UILayer> {
  public:
    static UILayerRef Make() { return base::make_ref<UILayer>(); }

    UILayer() = default;

    void reset();

    os::SurfaceRef surface() const { return m_surface; }
    gfx::Point& position() { return m_pos; }
    os::Paint& paint() { return m_paint; }
    const gfx::Region& clipRegion() const { return m_clip; }

    void setSurface(const os::SurfaceRef& newSurface);
    void setPosition(const gfx::Point& pos);
    void setPaint(const os::Paint p) { m_paint = p; }
    void setClipRegion(const gfx::Region& rgn) { m_clip = rgn; }

    gfx::Rect bounds() const;

    operator bool() const { return m_surface != nullptr; }

  private:
    os::SurfaceRef m_surface;
    gfx::Point m_pos;
    os::Paint m_paint;
    gfx::Region m_clip;
  };

} // namespace ui

#endif
