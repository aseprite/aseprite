/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_set_palette.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/file_selector.h"
#include "base/compiler_specific.h"
#include "base/unique_ptr.h"
#include "raster/palette.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class LoadPaletteCommand : public Command {
public:
  LoadPaletteCommand();
  Command* clone() const OVERRIDE { return new LoadPaletteCommand(*this); }

protected:
  void onExecute(Context* context) OVERRIDE;
};

LoadPaletteCommand::LoadPaletteCommand()
  : Command("LoadPalette",
            "Load Palette",
            CmdRecordableFlag)
{
}

void LoadPaletteCommand::onExecute(Context* context)
{
  std::string filename = app::show_file_selector("Load Palette", "", "png,pcx,bmp,tga,lbm,col,gpl");
  if (!filename.empty()) {
    base::UniquePtr<raster::Palette> palette(raster::Palette::load(filename.c_str()));
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
