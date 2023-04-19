// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_GRID_H_INCLUDED
#define DOC_GRID_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "gfx/fwd.h"
#include "gfx/point.h"
#include "gfx/size.h"

namespace doc {

  class Grid {
  public:
    explicit Grid(const gfx::Size& sz = gfx::Size(16, 16))
      : m_tileSize(sz)
      , m_origin(0, 0)
      , m_tileCenter(sz.w/2, sz.h/2)
      , m_tileOffset(sz)
      , m_oddRowOffset(0, 0)
      , m_oddColOffset(0, 0)
      , m_mask(nullptr) { }

    explicit Grid(const gfx::Rect& rc)
      : m_tileSize(rc.size())
      , m_origin(rc.origin())
      , m_tileCenter(m_tileSize.w/2, m_tileSize.h/2)
      , m_tileOffset(m_tileSize)
      , m_oddRowOffset(0, 0)
      , m_oddColOffset(0, 0)
      , m_mask(nullptr) { }

    static Grid MakeRect(const gfx::Size& sz);
    static Grid MakeRect(const gfx::Rect& rc);

    bool isEmpty() const { return m_tileSize.w == 0 || m_tileSize.h == 0; }

    gfx::Size tileSize() const { return m_tileSize; }
    gfx::Point origin() const { return m_origin; }
    gfx::Point tileCenter() const { return m_tileCenter; }
    gfx::Point tileOffset() const { return m_tileOffset; }
    gfx::Point oddRowOffset() const { return m_oddRowOffset; }
    gfx::Point oddColOffset() const { return m_oddColOffset; }

    void tileSize(const gfx::Size& tileSize) { m_tileSize = tileSize; }
    void origin(const gfx::Point& origin) { m_origin = origin; }
    void tileCenter(const gfx::Point& tileCenter) { m_tileCenter = tileCenter; }
    void tileOffset(const gfx::Point& tileOffset) { m_tileOffset = tileOffset; }
    void oddRowOffset(const gfx::Point& oddRowOffset) { m_oddRowOffset = oddRowOffset; }
    void oddColOffset(const gfx::Point& oddColOffset) { m_oddColOffset = oddColOffset; }

    // Converts a tile position into a canvas position
    gfx::Point tileToCanvas(const gfx::Point& tile) const;
    gfx::Rect tileToCanvas(const gfx::Rect& tileBounds) const;
    gfx::Region tileToCanvas(const gfx::Region& tileRgn);

    // Converts a canvas position/rectangle into a tile position
    gfx::Point canvasToTile(const gfx::Point& canvasPoint) const;
    gfx::Rect canvasToTile(const gfx::Rect& canvasBounds) const;
    gfx::Region canvasToTile(const gfx::Region& canvasRgn);

    gfx::Size tilemapSizeToCanvas(const gfx::Size& tilemapSize) const;
    gfx::Rect tileBoundsInCanvas(const gfx::Point& tile) const;
    gfx::Rect alignBounds(const gfx::Rect& bounds) const;

    bool hasMask() const { return m_mask.get() != nullptr; }
    doc::ImageRef mask() const { return m_mask; }
    void setMask(const doc::ImageRef& mask) { m_mask = mask; }

    // Returns an array of tile positions that are touching the given region in the canvas
    std::vector<gfx::Point> tilesInCanvasRegion(const gfx::Region& rgn) const;

  private:
    gfx::Size m_tileSize;
    gfx::Point m_origin;
    gfx::Point m_tileCenter;
    gfx::Point m_tileOffset;
    gfx::Point m_oddRowOffset;
    gfx::Point m_oddColOffset;
    doc::ImageRef m_mask;
  };

} // namespace doc

#endif
