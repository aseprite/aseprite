// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
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
#include "ui/alert.h"

namespace app {

using namespace ui;

class LoadPaletteCommand : public Command {
public:
  LoadPaletteCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string m_preset;
  std::string m_filename;
  bool m_ui = true;
};

LoadPaletteCommand::LoadPaletteCommand() : Command(CommandId::LoadPalette())
{
}

void LoadPaletteCommand::onLoadParams(const Params& params)
{
  if (params.has_param("ui"))
    m_ui = params.get_as<bool>("ui");
  else
    m_ui = true;
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
  else if (context->isUIAvailable() && m_ui) {
    const base::paths exts = get_readable_palette_extensions();
    base::paths filenames;
    if (app::show_file_selector(Strings::load_palette_title(),
                                m_filename,
                                exts,
                                FileSelectorType::Open,
                                filenames)) {
      filename = filenames.front();
    }
  }
  else if (!m_filename.empty()) {
    filename = m_filename;
  }

  // Do nothing
  if (filename.empty())
    return;

  std::unique_ptr<doc::Palette> palette(load_palette(filename.c_str()));
  if (!palette) {
    if (context->isUIAvailable())
      ui::Alert::show(Strings::alerts_error_loading_file(filename));
    return;
  }

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    Commands::instance()->byId(CommandId::SetPalette()));
  cmd->setPalette(palette.get());
  context->executeCommand(cmd);
}

std::string LoadPaletteCommand::onGetFriendlyName() const
{
  std::string name = Command::onGetFriendlyName();
  if (m_preset == "default")
    name = Strings::commands_LoadDefaultPalette();
  return name;
}

Command* CommandFactory::createLoadPaletteCommand()
{
  return new LoadPaletteCommand;
}

} // namespace app
