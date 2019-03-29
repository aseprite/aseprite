// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
  ToggleTilesModeCommand()
    : Command(CommandId::ToggleTilesMode(), CmdUIOnlyFlag) {
  }

protected:
  bool onChecked(Context* context) override {
    auto colorBar = ColorBar::instance();
    return colorBar->inTilesMode();
  }

  void onExecute(Context* context) override {
    auto colorBar = ColorBar::instance();
    colorBar->setTilesMode(!colorBar->inTilesMode());
  }
};

Command* CommandFactory::createToggleTilesModeCommand()
{
  return new ToggleTilesModeCommand;
}

} // namespace app
