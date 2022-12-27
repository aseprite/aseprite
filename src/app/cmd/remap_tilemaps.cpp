// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remap_tilemaps.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/remap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"

namespace app {
namespace cmd {

using namespace doc;

RemapTilemaps::RemapTilemaps(Tileset* tileset,
                             const Remap& remap)
  : WithTileset(tileset)
  , m_remap(remap)
{
}

void RemapTilemaps::onExecute()
{
  Tileset* tileset = this->tileset();
  remapTileset(tileset, m_remap);
  incrementVersions(tileset);
}

void RemapTilemaps::onUndo()
{
  Tileset* tileset = this->tileset();
  remapTileset(tileset, m_remap.invert());
  incrementVersions(tileset);
}

void RemapTilemaps::remapTileset(Tileset* tileset, const Remap& remap)
{
  Sprite* spr = tileset->sprite();
  spr->remapTilemaps(tileset, remap);

  Doc* doc = static_cast<Doc*>(spr->document());
  DocEvent ev(doc);
  ev.sprite(spr);
  ev.tileset(tileset);
  doc->notify_observers<DocEvent&, const Remap&>(&DocObserver::onRemapTileset, ev, remap);
}

void RemapTilemaps::incrementVersions(Tileset* tileset)
{
  Sprite* spr = tileset->sprite();
  for (const Cel* cel : spr->uniqueCels()) {
    if (cel->layer()->isTilemap() &&
        static_cast<LayerTilemap*>(cel->layer())->tileset() == tileset) {
      cel->image()->incrementVersion();
    }
  }
}

} // namespace cmd
} // namespace app
