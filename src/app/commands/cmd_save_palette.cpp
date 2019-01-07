// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/file/palette_file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "base/fs.h"
#include "doc/palette.h"
#include "fmt/format.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class SavePaletteCommand : public Command {
public:
  SavePaletteCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_preset;
};

SavePaletteCommand::SavePaletteCommand()
  : Command(CommandId::SavePalette(), CmdRecordableFlag)
{
}

void SavePaletteCommand::onLoadParams(const Params& params)
{
  m_preset = params.get("preset");
}

void SavePaletteCommand::onExecute(Context* context)
{
  const doc::Palette* palette = get_current_palette();
  std::string filename;

  if (!m_preset.empty()) {
    filename = get_preset_palette_filename(m_preset, ".ase");
  }
  else {
    base::paths exts = get_writable_palette_extensions();
    base::paths selFilename;
    if (!app::show_file_selector(
          "Save Palette", "", exts,
          FileSelectorType::Save, selFilename))
      return;

    filename = selFilename.front();
  }

  if (!save_palette(filename.c_str(), palette, 16)) // TODO 16 should be configurable
    ui::Alert::show(fmt::format(Strings::alerts_error_saving_file(), filename));

  if (m_preset == get_default_palette_preset_name()) {
    set_default_palette(palette);
    if (!context->activeDocument())
      set_current_palette(palette, false);
  }
}

Command* CommandFactory::createSavePaletteCommand()
{
  return new SavePaletteCommand;
}

} // namespace app
