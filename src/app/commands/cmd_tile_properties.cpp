// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tile_data.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/tile_window.h"
#include "base/convert_to.h"
#include "doc/tileset.h"
#include "doc/user_data.h"

namespace app {

using namespace ui;

class TilePropertiesCommand : public Command {
public:
  TilePropertiesCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  doc::tile_index m_tileIndex;
};

TilePropertiesCommand::TilePropertiesCommand()
  : Command(CommandId::TileProperties())
  , m_tileIndex(-1)
{
}

void TilePropertiesCommand::onLoadParams(const Params& params)
{
  std::string index = params.get("index");
  if (!index.empty())
    m_tileIndex = base::convert_to<doc::tile_index>(index);
  else
    m_tileIndex = -1;
}

bool TilePropertiesCommand::onEnabled(Context* context)
{
  return context->isUIAvailable() && context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                                                         ContextFlags::ActiveLayerIsTilemap);
}

void TilePropertiesCommand::onExecute(Context* context)
{
  ContextReader reader(context);
  Tileset* tileset = reader.site().tileset();

  if (tileset == nullptr || m_tileIndex < 0 || m_tileIndex >= tileset->size())
    return;

  TileWindow window(tileset, m_tileIndex);
  if (!window.show())
    return;

  ContextWriter writer(reader);
  Tx tx(writer, friendlyName());

  // Change user data
  doc::UserData userData = window.userDataValue();
  if (tileset->getTileData(m_tileIndex) != userData) {
    tx(new cmd::SetTileData(tileset, m_tileIndex, userData));
  }

  tx.commit();
}

Command* CommandFactory::createTilePropertiesCommand()
{
  return new TilePropertiesCommand;
}

} // namespace app
