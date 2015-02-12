// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/file/palette_file.h"
#include "app/file_selector.h"
#include "app/modules/palettes.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/palette.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class SavePaletteCommand : public Command {
public:
  SavePaletteCommand();
  Command* clone() const override { return new SavePaletteCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

SavePaletteCommand::SavePaletteCommand()
  : Command("SavePalette",
            "Save Palette",
            CmdRecordableFlag)
{
}

void SavePaletteCommand::onExecute(Context* context)
{
  char exts[4096];
  get_writable_palette_extensions(exts, sizeof(exts));

  std::string filename;
  int ret;

again:
  filename = app::show_file_selector("Save Palette", "", exts);
  if (!filename.empty()) {
    if (base::is_file(filename)) {
      ret = Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
                        base::get_file_name(filename).c_str());

      if (ret == 2)
        goto again;
      else if (ret != 1)
        return;
    }

    doc::Palette* palette = get_current_palette();
    if (!save_palette(filename.c_str(), palette)) {
      Alert::show("Error<<Saving palette file||&Close");
    }
  }
}

Command* CommandFactory::createSavePaletteCommand()
{
  return new SavePaletteCommand;
}

} // namespace app
