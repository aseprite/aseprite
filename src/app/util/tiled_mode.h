// Aseprite
// Copyright (C) 2016-2025  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
#ifndef APP_TILED_MODE_H_INCLUDED
#define APP_TILED_MODE_H_INCLUDED
#pragma once

#include "app/transformation.h"
#include "doc/sprite.h"
#include "filters/tiled_mode.h"
#include "render/projection.h"

using namespace filters;

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

  bool hasModeFlag(filters::TiledMode mode) const { return (int(m_mode) & int(mode)) == int(mode); }

  // Returns true if some of the tiled modes are enabled.
  bool tiledEnabled() const
  {
    return hasModeFlag(TiledMode::X_AXIS) || hasModeFlag(TiledMode::Y_AXIS);
  }

  gfx::Size canvasSize() const;

  gfx::Point mainTilePosition() const;

  void expandRegionByTiledMode(gfx::Region& rgn, const render::Projection* proj = nullptr) const;

  void collapseRegionByTiledMode(gfx::Region& rgn) const;

  void wrapPosition(gfx::Region& rgn) const;

  // Wraps around the position of the transformation according to the canvas size,
  // including its pivot.
  void wrapTransformation(Transformation* transformation) const;

  // Draws copies of the image at the center of the dst Image over itself according
  // to the current tiled mode. It accommodates the copies at each side/corner as needed.
  // IMPORTANT: For using this function properly, the dst Image must have the
  // appropriate width and height, as shown in the following examples:
  // For the following dst images and a m_canvas with size `cw` and `ch`, depending
  // on the current tiled mode:
  //   dst Image for tiled mode BOTH:             dst Image for tiled mode Y_AXIS:
  // +-------------------------------------+          +-------+
  // |                  A                  |          |   A   |
  // |                  |                  |          |   |   |
  // |                  |                  |          |   |   |
  // |                  ch                 |          |   ch  |
  // |                  |                  |          |   |   |
  // |                  V                  |          |   V   |
  // |              +-------+              |          +-------+
  // |              |       |              |          |       |
  // |<---- cw ---->|  img  |<---- cw ---->|          |  img  |
  // |              |       |              |          |       |
  // |              +-------+              |          +-------+
  // |                  A                  |          |   A   |
  // |                  |                  |          |   |   |
  // |                  |                  |          |   |   |
  // |                  ch                 |          |   ch  |
  // |                  |                  |          |   |   |
  // |                  V                  |          |   V   |
  // +-------------------------------------+          +-------+
  //
  // After using this function:
  // +-------+------+-------+------+-------+          +-------+
  // |  img  |      |  img  |      |  img  |          |  img  |
  // |  copy |      |  copy |      |  copy |          |  copy |
  // |       |      |       |      |       |          |       |
  // +-------+      +-------+      +-------+          +-------+
  // |                                     |          |       |
  // |                                     |          |       |
  // +-------+      +-------+      +-------+          +-------+
  // |  img  |      |       |      |  img  |          |       |
  // |  copy |      |  img  |      |  copy |          |  img  |
  // |       |      |       |      |       |          |       |
  // +-------+      +-------+      +-------+          +-------+
  // |                                     |          |       |
  // |                                     |          |       |
  // +-------+      +-------+      +-------+          +-------+
  // |  img  |      |  img  |      |  img  |          |  img  |
  // |  copy |      |  copy |      |  copy |          |  copy |
  // |       |      |       |      |       |          |       |
  // +-------+------+-------+------+-------+          +-------+
  void drawTiled(doc::Image* dst) const;

private:
  filters::TiledMode m_mode;
  const doc::Sprite* m_canvas;
};

} // namespace app

#endif
