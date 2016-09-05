// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "app/modules/palettes.h"
#include "base/fs.h"
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
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_preset;
  std::string m_filename;
};

LoadPaletteCommand::LoadPaletteCommand()
  : Command("LoadPalette",
            "Load Palette",
            CmdRecordableFlag)
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
  else {
    std::string exts = get_readable_palette_extensions();
    filename = app::show_file_selector("Load Palette", "", exts,
                                       FileSelectorType::Open);
  }

  // Do nothing
  if (filename.empty())
    return;

  base::UniquePtr<doc::Palette> palette(load_palette(filename.c_str()));
  if (!palette) {
    if (context->isUIAvailable())
      Alert::show("Error<<Loading palette file||&Close");
    return;
  }

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    CommandsModule::instance()->getCommandByName(CommandId::SetPalette));
  cmd->setPalette(palette);
  context->executeCommand(cmd);
}

Command* CommandFactory::createLoadPaletteCommand()
{
  return new LoadPaletteCommand;
}

} // namespace app
