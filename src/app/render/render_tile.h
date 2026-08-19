// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_RENDER_TILE_H_INCLUDED
#define APP_RENDER_RENDER_TILE_H_INCLUDED
#pragma once

#include "doc/object_id.h"
#include "os/surface.h"
#include "ui/base.h"

#include <map>
#include <memory>
#include <vector>

namespace app {

using RenderTileId = uint32_t;

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

  // Returns the set of cached tiles for the given Sprite ID.
  CachedTiles& cachedTiles(doc::ObjectId id);

  // Clear the doc CachedTiles and moves all the surfaces to the list
  // of free surfaces m_freeTiles.
  void clearCachedTiles(doc::ObjectId id);

private:
  struct Cache {
    CachedTiles cachedTiles;
  };

  std::vector<os::SurfaceRef> m_freeTiles;
  std::map<doc::ObjectId, std::unique_ptr<Cache>> m_sprites;
};

} // namespace app

#endif // APP_RENDER_RENDER_TILE_H_INCLUDED
