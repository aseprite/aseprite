// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/palette_file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "base/fs.h"
#include "doc/palette.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

class SavePaletteCommand : public Command {
public:
  SavePaletteCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string m_preset;
  bool m_saveAsPreset = false;
};

SavePaletteCommand::SavePaletteCommand() : Command(CommandId::SavePalette(), CmdRecordableFlag)
{
}

void SavePaletteCommand::onLoadParams(const Params& params)
{
  m_preset = params.get("preset");
  m_saveAsPreset = (params.get("saveAsPreset") == "true");
}

void SavePaletteCommand::onExecute(Context* ctx)
{
  const doc::Palette* palette = get_current_palette();
  std::string filename;

  if (!m_preset.empty()) {
    filename = get_preset_palette_filename(m_preset, ".ase");
  }
  else {
    base::paths exts = get_writable_palette_extensions();
    base::paths selFilename;
    std::string initialPath = (m_saveAsPreset ? get_preset_palettes_dir() : "");
    if (!app::show_file_selector(Strings::save_palette_title(),
                                 initialPath,
                                 exts,
                                 FileSelectorType::Save,
                                 selFilename))
      return;

    filename = selFilename.front();

    // Check that the file format supports saving indexed images/color
    // palettes (e.g. if the user specified .jpg we should show that
    // that file format is not supported to save color palettes)
    if (!base::has_file_extension(filename, exts)) {
      if (ctx->isUIAvailable()) {
        ui::Alert::show(
          Strings::alerts_file_format_doesnt_support_palette(base::get_file_extension(filename)));
      }
      return;
    }
  }
  gfx::ColorSpaceRef colorSpace = nullptr;
  auto activeDoc = ctx->activeDocument();
  if (activeDoc)
    colorSpace = activeDoc->sprite()->colorSpace();

  if (!save_palette(filename.c_str(), palette, 16, colorSpace)) // TODO 16 should be configurable
    ui::Alert::show(Strings::alerts_error_saving_file(filename));

  if (m_preset == get_default_palette_preset_name()) {
    set_default_palette(palette);
    if (!activeDoc)
      set_current_palette(palette, false);
  }
  if (m_saveAsPreset) {
    App::instance()->PalettePresetsChange();
  }
}

std::string SavePaletteCommand::onGetFriendlyName() const
{
  if (m_preset == "default")
    return Strings::commands_SavePaletteAsDefault();
  else if (m_saveAsPreset)
    return Strings::commands_SavePaletteAsPreset();
  return Command::onGetFriendlyName();
}

Command* CommandFactory::createSavePaletteCommand()
{
  return new SavePaletteCommand;
}

} // namespace app
