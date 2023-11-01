// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tileset_match_flags.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/tileset.h"

namespace app {
namespace cmd {

SetTilesetMatchFlags::SetTilesetMatchFlags(Tileset* tileset,
                                           const tile_flags matchFlags)
  : WithTileset(tileset)
  , m_oldMatchFlags(tileset->matchFlags())
  , m_newMatchFlags(matchFlags)
{
}

void SetTilesetMatchFlags::onExecute()
{
  auto ts = tileset();
  ts->setMatchFlags(m_newMatchFlags);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

void SetTilesetMatchFlags::onUndo()
{
  auto ts = tileset();
  ts->setMatchFlags(m_oldMatchFlags);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

} // namespace cmd
} // namespace app
