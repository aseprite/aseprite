// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/ui/color_bar.h"

namespace app {

class PaletteEditorCommand : public Command {
public:
  PaletteEditorCommand();
  Command* clone() const override { return new PaletteEditorCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;

private:
  bool m_open;
  bool m_close;
  bool m_switch;
  bool m_background;
};

PaletteEditorCommand::PaletteEditorCommand()
  : Command("PaletteEditor",
            "Edit Palette",
            CmdRecordableFlag)
{
  m_open = true;
  m_close = false;
  m_switch = false;
  m_background = false;
}

void PaletteEditorCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;

  std::string open_str = params.get("open");
  if (open_str == "true") m_open = true;
  else m_open = false;

  std::string close_str = params.get("close");
  if (close_str == "true") m_close = true;
  else m_close = false;

  std::string switch_str = params.get("switch");
  if (switch_str == "true") m_switch = true;
  else m_switch = false;
}

bool PaletteEditorCommand::onChecked(Context* context)
{
  return ColorBar::instance()->inEditMode();
}

void PaletteEditorCommand::onExecute(Context* context)
{
  bool state = ColorBar::instance()->inEditMode();

  if (m_switch)
    state = !state;
  else if (m_open)
    state = true;
  else if (m_close)
    state = false;

  ColorBar::instance()->setEditMode(state);
}

Command* CommandFactory::createPaletteEditorCommand()
{
  return new PaletteEditorCommand;
}

} // namespace app
