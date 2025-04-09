// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/grid.h"

#include "doc/algo.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace doc {

// static
Grid Grid::MakeRect(const gfx::Size& sz)
{
  return Grid(sz);
}

// static
Grid Grid::MakeRect(const gfx::Rect& rc)
{
  return Grid(rc);
}

// Converts a tile position into a canvas position
gfx::Point Grid::tileToCanvas(const gfx::Point& tile) const
{
  gfx::Point result;
  result.x = tile.x * m_tileOffset.x + m_origin.x;
  result.y = tile.y * m_tileOffset.y + m_origin.y;
  if (tile.y & 1) // Odd row
    result += m_oddRowOffset;
  if (tile.x & 1) // Odd column
    result += m_oddColOffset;
  return result;
}

gfx::Rect Grid::tileToCanvas(const gfx::Rect& tileBounds) const
{
  gfx::Point pt1 = tileToCanvas(tileBounds.origin());
  gfx::Point pt2 = tileToCanvas(tileBounds.point2());
  return gfx::Rect(pt1, pt2);
}

gfx::Region Grid::tileToCanvas(const gfx::Region& tileRgn)
{
  gfx::Region canvasRgn;
  for (const gfx::Rect& rc : tileRgn) {
    canvasRgn |= gfx::Region(tileToCanvas(rc));
  }
  return canvasRgn;
}

gfx::Point Grid::canvasToTile(const gfx::Point& canvasPoint) const
{
  ASSERT(m_tileSize.w > 0);
  ASSERT(m_tileSize.h > 0);
  if (m_tileSize.w < 1 || m_tileSize.h < 1)
    return canvasPoint;

  gfx::Point tile;
  std::div_t divx = std::div((canvasPoint.x - m_origin.x), m_tileSize.w);
  std::div_t divy = std::div((canvasPoint.y - m_origin.y), m_tileSize.h);
  tile.x = divx.quot;
  tile.y = divy.quot;
  if (canvasPoint.x < m_origin.x && divx.rem)
    --tile.x;
  if (canvasPoint.y < m_origin.y && divy.rem)
    --tile.y;

  if (m_oddRowOffset.x != 0 || m_oddRowOffset.y != 0 || m_oddColOffset.x != 0 ||
      m_oddColOffset.y != 0) {
    gfx::Point bestTile = tile;
    int bestDist = std::numeric_limits<int>::max();
    for (int v = -1; v <= 2; ++v) {
      for (int u = -1; u <= 2; ++u) {
        gfx::Point neighbor(tile.x + u, tile.y + v);
        gfx::Point neighborCanvas = tileToCanvas(neighbor);

        if (hasMask()) {
          if (doc::get_pixel(m_mask.get(),
                             canvasPoint.x - neighborCanvas.x,
                             canvasPoint.y - neighborCanvas.y))
            return neighbor;
        }

        gfx::Point delta = neighborCanvas + m_tileCenter - canvasPoint;
        int dist = delta.x * delta.x + delta.y * delta.y;
        if (bestDist > dist) {
          bestDist = dist;
          bestTile = neighbor;
        }
      }
    }
    tile = bestTile;
  }

  return tile;
}

gfx::Rect Grid::canvasToTile(const gfx::Rect& canvasBounds) const
{
  gfx::Point pt1 = canvasToTile(canvasBounds.origin());
  gfx::Point pt2 = canvasToTile(gfx::Point(canvasBounds.x2() - 1, canvasBounds.y2() - 1));
  return gfx::Rect(pt1, gfx::Size(pt2.x - pt1.x + 1, pt2.y - pt1.y + 1));
}

gfx::Region Grid::canvasToTile(const gfx::Region& canvasRgn)
{
  gfx::Region tilesRgn;
  for (const gfx::Rect& rc : canvasRgn) {
    tilesRgn |= gfx::Region(canvasToTile(rc));
  }
  return tilesRgn;
}

gfx::Size Grid::tilemapSizeToCanvas(const gfx::Size& tilemapSize) const
{
  return gfx::Size(tilemapSize.w * m_tileSize.w, tilemapSize.h * m_tileSize.h);
}

gfx::Rect Grid::tileBoundsInCanvas(const gfx::Point& tile) const
{
  return gfx::Rect(tileToCanvas(tile), m_tileSize);
}

gfx::Rect Grid::alignBounds(const gfx::Rect& bounds) const
{
  gfx::Point pt1 = canvasToTile(bounds.origin());
  gfx::Point pt2 = canvasToTile(gfx::Point(bounds.x2() - 1, bounds.y2() - 1));
  return tileBoundsInCanvas(pt1) | tileBoundsInCanvas(pt2);
}

std::vector<gfx::Point> Grid::tilesInCanvasRegion(const gfx::Region& rgn) const
{
  std::vector<gfx::Point> result;
  if (rgn.isEmpty())
    return result;

  const gfx::Rect bounds = canvasToTile(rgn.bounds());
  if (bounds.w < 1 || bounds.h < 1)
    return result;

  ImageRef tmp(Image::create(IMAGE_BITMAP, bounds.w, bounds.h));
  const gfx::Rect tmpBounds = tmp->bounds();
  tmp->clear(0);

  for (const gfx::Rect& rc : rgn) {
    gfx::Rect tileBounds = canvasToTile(rc);
    tileBounds.x -= bounds.x;
    tileBounds.y -= bounds.y;
    tileBounds &= tmpBounds;
    if (!tileBounds.isEmpty())
      tmp->fillRect(tileBounds.x, tileBounds.y, tileBounds.x2() - 1, tileBounds.y2() - 1, 1);
  }

  const LockImageBits<BitmapTraits> bits(tmp.get());
  for (auto it = bits.begin(), end = bits.end(); it != end; ++it) {
    if (*it)
      result.push_back(gfx::Point(it.x() + bounds.x, it.y() + bounds.y));
  }
  return result;
}

Grid::IsometricGuide::IsometricGuide(const gfx::Size& sz)
{
  evenWidth = sz.w % 2 == 0;
  evenHeight = sz.h % 2 == 0;
  evenHalfWidth = ((sz.w / 2) % 2) == 0;
  evenHalfHeight = ((sz.h / 2) % 2) == 0;
  squareRatio = ABS(sz.w - sz.h) <= 1;
  oddSize = !evenWidth && evenHalfWidth && !evenHeight && evenHalfHeight;

  // TODO: add 'share edges' checkbox to UI.
  // For testing the option, set this 'false' to 'true'
  shareEdges = !(false && !squareRatio && evenHeight);

  start.x = 0;
  start.y = std::round(sz.h * 0.5);
  end.x = sz.w / 2;
  end.y = sz.h;

  if (!squareRatio) {
    if (evenWidth) {
      end.x--;
    }
    else if (oddSize) {
      start.x--;
      end.x++;
    }
  }
  if (!shareEdges) {
    start.y++;
  }
}

static void push_isometric_line_point(int x, int y, std::vector<gfx::Point>* data)
{
  if (data->empty() || data->back().y != y) {
    data->push_back(gfx::Point(x * int(x >= 0), y));
  }
}

std::vector<gfx::Point> Grid::getIsometricLine(const gfx::Size& sz)
{
  std::vector<gfx::Point> result;
  const IsometricGuide guide(sz);

  // We use the line drawing algorithm to find the points
  // for a single pixel-precise line
  doc::algo_line_continuous_with_fix_for_line_brush(guide.start.x,
                                                    guide.start.y,
                                                    guide.end.x,
                                                    guide.end.y,
                                                    &result,
                                                    (doc::AlgoPixel)&push_isometric_line_point);

  return result;
}

} // namespace doc
