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

#include "app/commands/command.h"
#include "app/file_selector.h"
#include "app/modules/palettes.h"
#include "base/compiler_specific.h"
#include "base/fs.h"
#include "base/path.h"
#include "raster/palette.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class SavePaletteCommand : public Command {
public:
  SavePaletteCommand();
  Command* clone() const OVERRIDE { return new SavePaletteCommand(*this); }

protected:
  void onExecute(Context* context) OVERRIDE;
};

SavePaletteCommand::SavePaletteCommand()
  : Command("SavePalette",
            "Save Palette",
            CmdRecordableFlag)
{
}

void SavePaletteCommand::onExecute(Context* context)
{
  std::string filename;
  int ret;

again:
  filename = app::show_file_selector("Save Palette", "", "png,pcx,bmp,tga,col,gpl");
  if (!filename.empty()) {
    if (base::is_file(filename)) {
      ret = Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
                        base::get_file_name(filename).c_str());

      if (ret == 2)
        goto again;
      else if (ret != 1)
        return;
    }

    raster::Palette* palette = get_current_palette();
    if (!palette->save(filename.c_str())) {
      Alert::show("Error<<Saving palette file||&Close");
    }
  }
}

Command* CommandFactory::createSavePaletteCommand()
{
  return new SavePaletteCommand;
}

} // namespace app
