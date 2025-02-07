// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
#ifndef APP_TILED_MODE_H_INCLUDED
#define APP_TILED_MODE_H_INCLUDED
#pragma once

#include "filters/tiled_mode.h"

#include "app/doc.h"
#include "gfx/region.h"
#include "render/projection.h"

namespace app {

class TiledModeHelper {
public:
  TiledModeHelper(const filters::TiledMode mode, const doc::Sprite* canvas)
    : m_mode(mode)
    , m_canvas(canvas)
  {
  }
  ~TiledModeHelper() {}

  void mode(const filters::TiledMode mode) { m_mode = mode; }

  gfx::Size canvasSize() const
  {
    gfx::Size sz(m_canvas->width(), m_canvas->height());
    if (int(m_mode) & int(filters::TiledMode::X_AXIS)) {
      sz.w += sz.w * 2;
    }
    if (int(m_mode) & int(filters::TiledMode::Y_AXIS)) {
      sz.h += sz.h * 2;
    }
    return sz;
  }

  gfx::Point mainTilePosition() const
  {
    gfx::Point pt(0, 0);
    if (int(m_mode) & int(filters::TiledMode::X_AXIS)) {
      pt.x += m_canvas->width();
    }
    if (int(m_mode) & int(filters::TiledMode::Y_AXIS)) {
      pt.y += m_canvas->height();
    }
    return pt;
  }

  void expandRegionByTiledMode(gfx::Region& rgn, const render::Projection* proj = nullptr) const
  {
    gfx::Region tile = rgn;
    const bool xTiled = (int(m_mode) & int(filters::TiledMode::X_AXIS));
    const bool yTiled = (int(m_mode) & int(filters::TiledMode::Y_AXIS));
    int w = m_canvas->width();
    int h = m_canvas->height();
    if (proj) {
      w = proj->applyX(w);
      h = proj->applyY(h);
    }
    if (xTiled) {
      tile.offset(w, 0);
      rgn |= tile;
      tile.offset(w, 0);
      rgn |= tile;
      tile.offset(-2 * w, 0);
    }
    if (yTiled) {
      tile.offset(0, h);
      rgn |= tile;
      tile.offset(0, h);
      rgn |= tile;
      tile.offset(0, -2 * h);
    }
    if (xTiled && yTiled) {
      tile.offset(w, h);
      rgn |= tile;
      tile.offset(w, 0);
      rgn |= tile;
      tile.offset(-w, h);
      rgn |= tile;
      tile.offset(w, 0);
      rgn |= tile;
    }
  }

  void collapseRegionByTiledMode(gfx::Region& rgn) const
  {
    auto canvasSize = this->canvasSize();
    rgn &= gfx::Region(gfx::Rect(canvasSize));

    const int sprW = m_canvas->width();
    const int sprH = m_canvas->height();

    gfx::Region newRgn;
    for (int v = 0; v < canvasSize.h; v += sprH) {
      for (int u = 0; u < canvasSize.w; u += sprW) {
        gfx::Region tmp(gfx::Rect(u, v, sprW, sprH));
        tmp &= rgn;
        tmp.offset(-u, -v);
        newRgn |= tmp;
      }
    }
    rgn = newRgn;
  }

  void wrapPosition(gfx::Region& rgn) const
  {
    if (int(m_mode) == int(filters::TiledMode::NONE))
      return;

    if (int(m_mode) & int(filters::TiledMode::X_AXIS))
      rgn.offset(m_canvas->width() * (1 - (rgn.bounds().x / m_canvas->width())), 0);

    if (int(m_mode) & int(filters::TiledMode::Y_AXIS))
      rgn.offset(0, m_canvas->height() * (1 - (rgn.bounds().y / m_canvas->height())));
  }

private:
  filters::TiledMode m_mode;
  const doc::Sprite* m_canvas;
};

} // namespace app

#endif
