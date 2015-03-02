// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_set_palette.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/file/palette_file.h"
#include "app/file_selector.h"
#include "base/unique_ptr.h"
#include "doc/palette.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class LoadPaletteCommand : public Command {
public:
  LoadPaletteCommand();
  Command* clone() const override { return new LoadPaletteCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

LoadPaletteCommand::LoadPaletteCommand()
  : Command("LoadPalette",
            "Load Palette",
            CmdRecordableFlag)
{
}

void LoadPaletteCommand::onExecute(Context* context)
{
  char exts[4096];
  get_readable_palette_extensions(exts, sizeof(exts));

  std::string filename =
    app::show_file_selector("Load Palette", "", exts,
      FileSelectorType::Open);

  if (!filename.empty()) {
    base::UniquePtr<doc::Palette> palette(load_palette(filename.c_str()));
    if (!palette) {
      Alert::show("Error<<Loading palette file||&Close");
    }
    else {
      SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
        CommandsModule::instance()->getCommandByName(CommandId::SetPalette));
      cmd->setPalette(palette);
      context->executeCommand(cmd);
    }
  }
}

Command* CommandFactory::createLoadPaletteCommand()
{
  return new LoadPaletteCommand;
}

} // namespace app
