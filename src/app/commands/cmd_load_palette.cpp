// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_set_palette.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
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

class LoadPaletteCommand : public Command {
public:
  LoadPaletteCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_preset;
  std::string m_filename;
};

LoadPaletteCommand::LoadPaletteCommand()
  : Command(CommandId::LoadPalette(), CmdRecordableFlag)
{
}

void LoadPaletteCommand::onLoadParams(const Params& params)
{
  m_preset = params.get("preset");
  m_filename = params.get("filename");
}

void LoadPaletteCommand::onExecute(Context* context)
{
  std::string filename;

  if (!m_preset.empty()) {
    filename = get_preset_palette_filename(m_preset, ".ase");
    if (!base::is_file(filename))
      filename = get_preset_palette_filename(m_preset, ".gpl");
  }
  else if (!m_filename.empty()) {
    filename = m_filename;
  }
#ifdef ENABLE_UI
  else if (context->isUIAvailable()) {
    base::paths exts = get_readable_palette_extensions();
    base::paths filenames;
    if (app::show_file_selector(
          Strings::load_palette_title(), "", exts,
          FileSelectorType::Open, filenames)) {
      filename = filenames.front();
    }
  }
#endif // ENABLE_UI

  // Do nothing
  if (filename.empty())
    return;

  std::unique_ptr<doc::Palette> palette(load_palette(filename.c_str()));
  if (!palette) {
    if (context->isUIAvailable())
      ui::Alert::show(fmt::format(Strings::alerts_error_loading_file(), filename));
    return;
  }

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    Commands::instance()->byId(CommandId::SetPalette()));
  cmd->setPalette(palette.get());
  context->executeCommand(cmd);
}

Command* CommandFactory::createLoadPaletteCommand()
{
  return new LoadPaletteCommand;
}

} // namespace app
