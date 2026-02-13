// Aseprite
// Copyright (C) 2016-2026  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/tiled_mode.h"

#include "doc/algorithm/rotate.h"
#include "doc/mask.h"
#include "gfx/region.h"

using namespace filters;
using namespace doc;

namespace app {

gfx::Size TiledModeHelper::canvasSize() const
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

gfx::Point TiledModeHelper::mainTilePosition() const
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

void TiledModeHelper::expandRegionByTiledMode(gfx::Region& rgn,
                                              const render::Projection* proj) const
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

void TiledModeHelper::collapseRegionByTiledMode(gfx::Region& rgn) const
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

void TiledModeHelper::wrapPosition(gfx::Region& rgn) const
{
  if (int(m_mode) == int(filters::TiledMode::NONE))
    return;

  if (int(m_mode) & int(filters::TiledMode::X_AXIS)) {
    int w = rgn.bounds().origin().x < 0 ? (rgn.bounds().w - m_canvas->width()) : 0;
    rgn.offset(m_canvas->width() - m_canvas->width() * ((rgn.bounds().x + w) / m_canvas->width()),
               0);
  }

  if (int(m_mode) & int(filters::TiledMode::Y_AXIS)) {
    int h = rgn.bounds().origin().y < 0 ? (rgn.bounds().h - m_canvas->height()) : 0;
    rgn.offset(
      0,
      m_canvas->height() - m_canvas->height() * ((rgn.bounds().y + h) / m_canvas->height()));
  }
}

// Wraps around the position of the transformation according to the canvas size,
// including its pivot.
void TiledModeHelper::wrapTransformation(Transformation* transformation) const
{
  gfx::Rect bounds = transformation->transformedBounds();
  // Wrap the transformed bounds position
  gfx::Region rgn(bounds);
  wrapPosition(rgn);
  // Center the wrapped bounds.
  if (hasModeFlag(TiledMode::X_AXIS))
    rgn.offset(-m_canvas->width(), 0);
  if (hasModeFlag(TiledMode::Y_AXIS))
    rgn.offset(0, -m_canvas->height());

  // Calculate position deltas between bounds positions before and after wrapping
  auto dx = rgn.bounds().x - bounds.x;
  auto dy = rgn.bounds().y - bounds.y;
  auto b = transformation->bounds();
  auto p = transformation->pivot();
  // Move transformation bounds and pivot by the deltas.
  transformation->bounds(b.offset(dx, dy));
  p += gfx::PointF(dx, dy);
  transformation->pivot(p);
}

void TiledModeHelper::drawTiled(doc::Image* dst, const gfx::Rect& cornerBounds) const
{
  if (!tiledEnabled())
    return;

  doc::ImageRef tmp;
  Mask tmpMask;
  tmpMask.freeze();
  const int x = (hasModeFlag(TiledMode::X_AXIS) ? m_canvas->width() : 0);
  const int y = (hasModeFlag(TiledMode::Y_AXIS) ? m_canvas->height() : 0);
  const int w = cornerBounds.w;
  const int h = cornerBounds.h;

  tmp.reset(Image::create(dst->pixelFormat(), w, h));
  tmp->copy(dst, gfx::Clip(0, 0, x, y, w, h));
  tmpMask.fromImage(tmp.get(), {});

  if (hasModeFlag(TiledMode::X_AXIS)) {
    // Draw at the left side
    doc::algorithm::parallelogram(dst, tmp.get(), tmpMask.bitmap(), 0, y, w, y, w, y + h, 0, y + h);

    // Draw at the right side
    doc::algorithm::parallelogram(dst,
                                  tmp.get(),
                                  tmpMask.bitmap(),
                                  dst->width() - w,
                                  y,
                                  dst->width(),
                                  y,
                                  dst->width(),
                                  y + h,
                                  dst->width() - w,
                                  y + h);
  }
  if (hasModeFlag(TiledMode::Y_AXIS)) {
    // Draw at the top.
    doc::algorithm::parallelogram(dst, tmp.get(), tmpMask.bitmap(), x, 0, x + w, 0, x + w, h, x, h);

    // Draw at the bottom.
    doc::algorithm::parallelogram(dst,
                                  tmp.get(),
                                  tmpMask.bitmap(),
                                  x,
                                  dst->height() - h,
                                  x + w,
                                  dst->height() - h,
                                  x + w,
                                  dst->height(),
                                  x,
                                  dst->height());
  }
  if (hasModeFlag(TiledMode::BOTH)) {
    // Draw at the top-left corner.
    doc::algorithm::parallelogram(dst, tmp.get(), tmpMask.bitmap(), 0, 0, w, 0, w, h, 0, h);
    // Draw at the bottom-left corner.
    doc::algorithm::parallelogram(dst,
                                  tmp.get(),
                                  tmpMask.bitmap(),
                                  0,
                                  dst->height() - h,
                                  w,
                                  dst->height() - h,
                                  w,
                                  dst->height(),
                                  0,
                                  dst->height());

    // Draw at the top-right corner.
    doc::algorithm::parallelogram(dst,
                                  tmp.get(),
                                  tmpMask.bitmap(),
                                  dst->width() - w,
                                  0,
                                  dst->width(),
                                  0,
                                  dst->width(),
                                  h,
                                  dst->width() - w,
                                  h);

    // Draw at the bottom-right corner.
    doc::algorithm::parallelogram(dst,
                                  tmp.get(),
                                  tmpMask.bitmap(),
                                  dst->width() - w,
                                  dst->height() - h,
                                  dst->width(),
                                  dst->height() - h,
                                  dst->width(),
                                  dst->height(),
                                  dst->width() - w,
                                  dst->height());
  }
  tmpMask.unfreeze();
}

} // namespace app
