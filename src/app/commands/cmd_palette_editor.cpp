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
#include "app/i18n/strings.h"
#include "app/ui/color_bar.h"
#include "fmt/format.h"

namespace app {

class PaletteEditorCommand : public Command {
public:
  PaletteEditorCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  bool m_edit;
  bool m_popup;
  bool m_background;
};

PaletteEditorCommand::PaletteEditorCommand()
  : Command(CommandId::PaletteEditor(), CmdRecordableFlag)
{
  m_edit = true;
  m_popup = false;
  m_background = false;
}

void PaletteEditorCommand::onLoadParams(const Params& params)
{
  m_edit =
    (params.empty() ||
     params.get("edit") == "switch" ||
     params.get_as<bool>("switch")); // "switch" for backward compatibility
  m_popup = (!params.get("popup").empty());
  m_background = (params.get("popup") == "background");
}

bool PaletteEditorCommand::onChecked(Context* context)
{
  return ColorBar::instance()->inEditMode();
}

void PaletteEditorCommand::onExecute(Context* context)
{
  auto colorBar = ColorBar::instance();
  bool editMode = colorBar->inEditMode();
  ColorButton* button =
    (m_background ?
     colorBar->bgColorButton():
     colorBar->fgColorButton());

  // Switch edit mode
  if (m_edit && !m_popup) {
    colorBar->setEditMode(!editMode);
  }
  // Switch popup
  else if (!m_edit && m_popup) {
    if (button->isPopupVisible())
      button->closePopup();
    else
      button->openPopup(true);
  }
  // Switch both (the popup visibility is used to switch both states)
  else if (m_edit && m_popup) {
    if (button->isPopupVisible()) {
      colorBar->setEditMode(false);
      button->closePopup();
    }
    else {
      colorBar->setEditMode(true);
      button->openPopup(true);
    }
  }
}

std::string PaletteEditorCommand::onGetFriendlyName() const
{
  std::string edit, plus, popup;
  if (m_edit) {
    edit = Strings::commands_PaletteEditor_Edit();
  }
  if (m_edit && m_popup) {
    plus = Strings::commands_PaletteEditor_And();
  }
  if (m_popup) {
    if (m_background)
      popup = Strings::commands_PaletteEditor_BgPopup();
    else
      popup = Strings::commands_PaletteEditor_FgPopup();
  }
  return fmt::format(getBaseFriendlyName(), edit, plus, popup);
}

Command* CommandFactory::createPaletteEditorCommand()
{
  return new PaletteEditorCommand;
}

} // namespace app
