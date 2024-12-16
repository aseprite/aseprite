// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <string>

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/ui/color_bar.h"
#include "doc/palette.h"

namespace app {

class ChangeColorCommand : public Command {
  enum Change {
    None,
    IncrementIndex,
    DecrementIndex,
  };

  /**
   * True means "change background color", false the foreground color.
   */
  bool m_background;

  Change m_change;

public:
  ChangeColorCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
};

ChangeColorCommand::ChangeColorCommand() : Command(CommandId::ChangeColor(), CmdUIOnlyFlag)
{
  m_background = false;
  m_change = None;
}

void ChangeColorCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "foreground")
    m_background = false;
  else if (target == "background")
    m_background = true;

  std::string change = params.get("change");
  if (change == "increment-index")
    m_change = IncrementIndex;
  else if (change == "decrement-index")
    m_change = DecrementIndex;
}

void ChangeColorCommand::onExecute(Context* context)
{
  ColorBar* colorbar = ColorBar::instance();
  app::Color color = m_background ? colorbar->getBgColor() : colorbar->getFgColor();

  switch (m_change) {
    case None:
      // do nothing
      break;
    case IncrementIndex: {
      const int palSize = get_current_palette()->size();
      if (palSize >= 1) {
        // Seems safe to assume getIndex() will return a
        // positive number, so use truncation modulo.
        color = app::Color::fromIndex((color.getIndex() + 1) % palSize);
      }
      break;
    }
    case DecrementIndex: {
      const int palSize = get_current_palette()->size();
      if (palSize >= 1) {
        // Floor modulo.
        color = app::Color::fromIndex(((color.getIndex() - 1) % palSize + palSize) % palSize);
      }
      break;
    }
  }

  if (m_background)
    colorbar->setBgColor(color);
  else
    colorbar->setFgColor(color);
}

std::string ChangeColorCommand::onGetFriendlyName() const
{
  std::string action;

  switch (m_change) {
    case None: break;
    case IncrementIndex:
      if (m_background)
        action = Strings::commands_ChangeColor_IncrementBgIndex();
      else
        action = Strings::commands_ChangeColor_IncrementFgIndex();
      break;
    case DecrementIndex:
      if (m_background)
        action = Strings::commands_ChangeColor_DecrementBgIndex();
      else
        action = Strings::commands_ChangeColor_DecrementFgIndex();
      break;
  }

  return Strings::commands_ChangeColor(action);
}

Command* CommandFactory::createChangeColorCommand()
{
  return new ChangeColorCommand;
}

} // namespace app
