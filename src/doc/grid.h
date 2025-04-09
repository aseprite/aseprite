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
  enum class Type {
    Orthogonal,
    Isometric,
  };
  explicit Grid(const gfx::Size& sz = gfx::Size(16, 16), const Type t = Type::Orthogonal)
    : m_tileSize(sz)
    , m_type(t)
    , m_origin(0, 0)
    , m_tileCenter(sz.w / 2, sz.h / 2)
    , m_tileOffset(sz)
    , m_oddRowOffset(0, 0)
    , m_oddColOffset(0, 0)
    , m_mask(nullptr)
  {
  }

  explicit Grid(const gfx::Rect& rc, const Type t = Type::Orthogonal)
    : m_tileSize(rc.size())
    , m_type(t)
    , m_origin(rc.origin())
    , m_tileCenter(m_tileSize.w / 2, m_tileSize.h / 2)
    , m_tileOffset(m_tileSize)
    , m_oddRowOffset(0, 0)
    , m_oddColOffset(0, 0)
    , m_mask(nullptr)
  {
  }

  static Grid MakeRect(const gfx::Size& sz);
  static Grid MakeRect(const gfx::Rect& rc);

  bool isEmpty() const { return m_tileSize.w == 0 || m_tileSize.h == 0; }

  gfx::Size tileSize() const { return m_tileSize; }
  Type type() const { return m_type; }
  gfx::Point origin() const { return m_origin; }
  gfx::Point tileCenter() const { return m_tileCenter; }
  gfx::Point tileOffset() const { return m_tileOffset; }
  gfx::Point oddRowOffset() const { return m_oddRowOffset; }
  gfx::Point oddColOffset() const { return m_oddColOffset; }
  gfx::Rect bounds() const { return gfx::Rect(origin(), tileSize()); }

  void tileSize(const gfx::Size& tileSize) { m_tileSize = tileSize; }
  void type(const Type t) { m_type = t; }
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

  // Helper structure for calculating both isometric grid cells
  // as well as point snapping
  struct IsometricGuide {
    gfx::Point start;
    gfx::Point end;
    bool evenWidth : 1;
    bool evenHeight : 1;
    bool evenHalfWidth : 1;
    bool evenHalfHeight : 1;
    bool squareRatio : 1;
    bool oddSize : 1;
    bool shareEdges : 1;

    IsometricGuide(const gfx::Size& sz);
  };

  // Returns an array of coordinates used for calculating the
  // pixel-precise bounds of an isometric grid cell
  static std::vector<gfx::Point> getIsometricLine(const gfx::Size& sz);

private:
  gfx::Size m_tileSize;
  Type m_type;
  gfx::Point m_origin;
  gfx::Point m_tileCenter;
  gfx::Point m_tileOffset;
  gfx::Point m_oddRowOffset;
  gfx::Point m_oddColOffset;
  doc::ImageRef m_mask;
};

} // namespace doc

#endif
