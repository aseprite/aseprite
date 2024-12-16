// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/ui/color_bar.h"

namespace app {

using namespace gfx;

class TilesetModeCommand : public Command {
public:
  TilesetModeCommand() : Command(CommandId::TilesetMode(), CmdUIOnlyFlag)
  {
    m_mode = TilesetMode::Auto;
  }

protected:
  void onLoadParams(const Params& params) override
  {
    std::string mode = params.get("mode");
    if (mode == "manual")
      m_mode = TilesetMode::Manual;
    else if (mode == "stack")
      m_mode = TilesetMode::Stack;
    else
      m_mode = TilesetMode::Auto;
  }

  bool onChecked(Context* context) override
  {
    auto colorBar = ColorBar::instance();
    return (colorBar->tilesetMode() == m_mode);
  }

  void onExecute(Context* context) override
  {
    auto colorBar = ColorBar::instance();
    colorBar->setTilesetMode(m_mode);
  }

  std::string onGetFriendlyName() const override
  {
    std::string mode;
    switch (m_mode) {
      case TilesetMode::Manual: mode = Strings::commands_TilesetMode_Manual(); break;
      case TilesetMode::Auto:   mode = Strings::commands_TilesetMode_Auto(); break;
      case TilesetMode::Stack:  mode = Strings::commands_TilesetMode_Stack(); break;
    }
    return Strings::commands_TilesetMode(mode);
  }

  bool isListed(const Params& params) const override { return !params.empty(); }

private:
  TilesetMode m_mode;
};

Command* CommandFactory::createTilesetModeCommand()
{
  return new TilesetModeCommand;
}

} // namespace app
