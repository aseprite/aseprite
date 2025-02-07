// Aseprite
// Copyright (c) 2019-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/color_bar.h"

namespace app {

using namespace gfx;

class ToggleTilesModeCommand : public Command {
public:
  ToggleTilesModeCommand() : Command(CommandId::ToggleTilesMode(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* context) override
  {
    auto colorBar = ColorBar::instance();
    return (colorBar->tilemapMode() == TilemapMode::Tiles);
  }

  void onExecute(Context* context) override
  {
    auto colorBar = ColorBar::instance();
    if (!colorBar->isTilemapModeLocked()) {
      colorBar->setTilemapMode(
        colorBar->tilemapMode() == TilemapMode::Pixels ? TilemapMode::Tiles : TilemapMode::Pixels);
    }
  }
};

Command* CommandFactory::createToggleTilesModeCommand()
{
  return new ToggleTilesModeCommand;
}

} // namespace app
