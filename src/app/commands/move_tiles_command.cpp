// Aseprite
// Copyright (c) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/util/cel_ops.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/remap.h"
#include "doc/tileset.h"

namespace app {

using namespace ui;

struct MoveTilesParams : public NewParams {
  Param<int> before{ this, 0, "before" };
};

class MoveTilesCommand : public CommandWithNewParams<MoveTilesParams> {
public:
  MoveTilesCommand(const bool copy)
    : CommandWithNewParams<MoveTilesParams>(
        (copy ? CommandId::CopyTiles() : CommandId::MoveTiles()),
        CmdRecordableFlag)
    , m_copy(copy)
  {
  }

protected:
  bool onEnabled(Context* ctx) override
  {
    return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveLayer |
                           ContextFlags::ActiveLayerIsTilemap);
  }

  void onExecute(Context* ctx) override
  {
    ContextWriter writer(ctx);
    doc::Layer* layer = writer.layer();
    if (!layer || !layer->isTilemap())
      return;

    doc::Tileset* tileset = static_cast<LayerTilemap*>(layer)->tileset();
    ASSERT(tileset);
    if (!tileset)
      return;

    PalettePicks picks = writer.site()->selectedTiles();
    if (picks.picks() == 0)
      return;

    Tx tx(writer, onGetFriendlyName(), ModifyDocument);
    const int beforeIndex = params().before();
    int currentEntry = picks.firstPick();

    if (m_copy)
      copy_tiles_in_tileset(tx, tileset, picks, currentEntry, beforeIndex);
    else
      move_tiles_in_tileset(tx, tileset, picks, currentEntry, beforeIndex);

    tx.commit();

    ctx->setSelectedTiles(picks);
  }

private:
  bool m_copy;
};

Command* CommandFactory::createMoveTilesCommand()
{
  return new MoveTilesCommand(false);
}

Command* CommandFactory::createCopyTilesCommand()
{
  return new MoveTilesCommand(true);
}

} // namespace app
