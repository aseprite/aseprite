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
#include "app/commands/commands.h"
#include "app/commands/new_params.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/palette_file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "base/fs.h"
#include "ui/alert.h"

namespace app {

using namespace ui;

struct SavePaletteParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<std::string> filename{ this, "", "filename" };
  Param<std::string> preset{ this, "", "preset" };
  Param<bool> saveAsPreset{ this, false, "saveAsPreset" };
  Param<int> columns{ this, 16, "columns" };
};

class SavePaletteCommand : public CommandWithNewParams<SavePaletteParams> {
public:
  SavePaletteCommand();

protected:
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

SavePaletteCommand::SavePaletteCommand() : CommandWithNewParams(CommandId::SavePalette())
{
}

void SavePaletteCommand::onExecute(Context* ctx)
{
  const doc::Palette* palette = get_current_palette();
  const bool ui = (params().ui() && ctx->isUIAvailable());
  std::string filename;

  if (!params().preset().empty()) {
    filename = get_preset_palette_filename(params().preset(), ".ase");
  }
  else {
    base::paths exts = get_writable_palette_extensions();

    if (ui) {
      base::paths selFilename;
      std::string initialPath = params().saveAsPreset() ? get_preset_palettes_dir() :
                                                          params().filename();
      if (!app::show_file_selector(Strings::save_palette_title(),
                                   initialPath,
                                   exts,
                                   FileSelectorType::Save,
                                   selFilename))
        return;

      filename = selFilename.front();
    }
    else {
      filename = params().filename();
    }

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
  auto* activeDoc = ctx->activeDocument();
  if (activeDoc)
    colorSpace = activeDoc->sprite()->colorSpace();

  const bool result = save_palette(filename.c_str(), palette, params().columns(), colorSpace);

  if (ui && !result)
    ui::Alert::show(Strings::alerts_error_saving_file(filename));

  if (params().preset() == get_default_palette_preset_name()) {
    set_default_palette(palette);
    if (!activeDoc)
      set_current_palette(palette, false);
  }

  if (params().saveAsPreset()) {
    App::instance()->PalettePresetsChange();
  }
}

std::string SavePaletteCommand::onGetFriendlyName() const
{
  if (params().preset() == get_default_palette_preset_name())
    return Strings::commands_SavePaletteAsDefault();
  if (params().saveAsPreset())
    return Strings::commands_SavePaletteAsPreset();
  return Command::onGetFriendlyName();
}

Command* CommandFactory::createSavePaletteCommand()
{
  return new SavePaletteCommand;
}

} // namespace app
