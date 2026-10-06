// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_RENDER_TILE_H_INCLUDED
#define APP_RENDER_RENDER_TILE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/object_id.h"
#include "os/surface.h"
#include "ui/base.h"

#include <map>
#include <memory>
#include <vector>

namespace app {

class CanvasView;

using RenderTileId = uint32_t;

inline RenderTileId make_render_tile_id(int u, int v, doc::frame_t f)
{
  return (u & 0xfff) | ((v & 0xfff) << 12) | ((f & 0xff) << 24);
}

inline int render_tile_u(RenderTileId id)
{
  return id & 0xfff;
}

inline int render_tile_v(RenderTileId id)
{
  return (id >> 12) & 0xfff;
}

inline int render_tile_frame(RenderTileId id)
{
  return (id >> 24) & 0xff;
}

struct RenderTile {
  constexpr static gfx::Size kTileSize = gfx::Size(128, 128);

  RenderTileId tileId = 0;
  gfx::Rect src, dst;
  os::SurfaceRef surface;
  bool dirty = true;

  RenderTile() {}

  RenderTile(const RenderTileId tileId, const gfx::Rect& src, const gfx::Rect& dst)
    : tileId(tileId)
    , src(src)
    , dst(dst)
  {
  }

  RenderTile(const RenderTile&) = default;
  RenderTile& operator=(const RenderTile&) = default;
};

using CachedTiles = std::map<RenderTileId, RenderTile>;

class RenderTileCache {
public:
  // Creates a new surface to be used for a RenderTile (or re-use a
  // free one available from m_freeTiles).
  os::SurfaceRef allocTileSurface();

  // Returns the set of cached tiles for the given canvas view.
  CachedTiles& cachedTiles(const CanvasView* view);

  // Clear the doc CachedTiles and moves all the surfaces to the list
  // of free surfaces m_freeTiles.
  void clearCachedTiles(const CanvasView* view, bool viewIsDeleted);

private:
  struct Cache {
    CachedTiles cachedTiles;
  };

  std::vector<os::SurfaceRef> m_freeTiles;
  std::map<const CanvasView*, std::unique_ptr<Cache>> m_views;
};

} // namespace app

#endif // APP_RENDER_RENDER_TILE_H_INCLUDED
