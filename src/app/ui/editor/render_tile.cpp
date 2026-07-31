// Aseprite
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/render_tile.h"

#include "os/system.h"

namespace app {

os::SurfaceRef RenderTileCache::allocTileSurface()
{
  if (m_freeTiles.empty())
    return os::System::instance()->makeRgbaSurface(RenderTile::kTileSize.w,
                                                   RenderTile::kTileSize.h);

  os::SurfaceRef surface = m_freeTiles.back();
  m_freeTiles.pop_back();
  return surface;
}

CachedTiles& RenderTileCache::docCachedTiles(const doc::ObjectId docId)
{
  auto it = m_docs.find(docId);
  if (it != m_docs.end())
    return it->second->cachedTiles;

  auto docCache = std::make_unique<DocCache>();
  CachedTiles& cachedTiles = docCache->cachedTiles;
  m_docs.emplace(docId, std::move(docCache));
  return cachedTiles;
}

void RenderTileCache::clearDocCachedTiles(doc::ObjectId docId)
{
  CachedTiles& cachedTiles = RenderTileCache::docCachedTiles(docId);
  for (auto& [tileId, cachedTile] : cachedTiles) {
    m_freeTiles.push_back(cachedTile.surface);
    cachedTile.surface = nullptr;
  }
  cachedTiles.clear();
}

} // namespace app
