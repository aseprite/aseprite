// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_tileset.h"

#include "app/cmd_exception.h"
#include "doc/tileset.h"

namespace app { namespace cmd {

using namespace doc;

WithTileset::WithTileset(Tileset* tileset) : m_tilesetId(tileset ? tileset->id() : NullId)
{
}

Tileset* WithTileset::tileset()
{
  Tileset* tileset = get<Tileset>(m_tilesetId);
  if (!tileset)
    throw CmdException(
      fmt::format("Invalid undo information: tileset ID {} not found", m_tilesetId));
  return tileset;
}

}} // namespace app::cmd
