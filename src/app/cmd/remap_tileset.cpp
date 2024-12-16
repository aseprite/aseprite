// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remap_tileset.h"

#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/remap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"

namespace app { namespace cmd {

using namespace doc;

RemapTileset::RemapTileset(Tileset* tileset, const Remap& remap)
  : WithTileset(tileset)
  , m_remap(remap)
{
}

void RemapTileset::onExecute()
{
  Tileset* tileset = this->tileset();
  applyRemap(tileset, m_remap);
}

void RemapTileset::onUndo()
{
  Tileset* tileset = this->tileset();
  applyRemap(tileset, m_remap.invert());
}

void RemapTileset::applyRemap(Tileset* tileset, const Remap& remap)
{
  tileset->remap(remap);
  tileset->incrementVersion();
  tileset->sprite()->incrementVersion();
}

}} // namespace app::cmd
