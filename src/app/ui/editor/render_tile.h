// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_RENDER_TILE_H_INCLUDED
#define APP_UI_EDITOR_RENDER_TILE_H_INCLUDED
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

  RenderTileId tileId;
  gfx::Rect src, dst;
  os::SurfaceRef surface;
  bool dirty = true;

  RenderTile(const RenderTileId tileId, const gfx::Rect& src, const gfx::Rect& dst)
    : tileId(tileId)
    , src(src)
    , dst(dst)
  {
  }
};

using CachedTiles = std::map<RenderTileId, RenderTile>;

class RenderTileCache {
public:
  // Creates a new surface to be used for a RenderTile (or re-use a
  // free one available from m_freeTiles).
  os::SurfaceRef allocTileSurface();

  CachedTiles& docCachedTiles(doc::ObjectId docId);

  // Clear the doc CachedTiles and moves all the surfaces to the list
  // of free surfaces m_freeTiles.
  void clearDocCachedTiles(doc::ObjectId docId);

private:
  struct DocCache {
    CachedTiles cachedTiles;
  };

  std::vector<os::SurfaceRef> m_freeTiles;
  std::map<doc::ObjectId, std::unique_ptr<DocCache>> m_docs;
};

} // namespace app

#endif // APP_UI_EDITOR_RENDER_TILE_H_INCLUDED
