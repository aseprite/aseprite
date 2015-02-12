// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/ui/color_bar.h"

namespace app {

class ChangeColorCommand : public Command
{
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
  void onLoadParams(Params* params);
  void onExecute(Context* context);
  std::string onGetFriendlyName() const;
};

ChangeColorCommand::ChangeColorCommand()
  : Command("ChangeColor",
            "Change Color",
            CmdUIOnlyFlag)
{
  m_background = false;
  m_change = None;
}

void ChangeColorCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;

  std::string change = params->get("change");
  if (change == "increment-index") m_change = IncrementIndex;
  else if (change == "decrement-index") m_change = DecrementIndex;
}

void ChangeColorCommand::onExecute(Context* context)
{
  ColorBar* colorbar = ColorBar::instance();
  app::Color color = m_background ? colorbar->getBgColor():
                                    colorbar->getFgColor();

  switch (m_change) {
    case None:
      // do nothing
      break;
    case IncrementIndex:
      if (color.getType() == app::Color::IndexType) {
        int index = color.getIndex();
        if (index < 255)        // TODO use sprite palette limit
          color = app::Color::fromIndex(index+1);
      }
      else
        color = app::Color::fromIndex(0);
      break;
    case DecrementIndex:
      if (color.getType() == app::Color::IndexType) {
        int index = color.getIndex();
        if (index > 0)
          color = app::Color::fromIndex(index-1);
      }
      else
        color = app::Color::fromIndex(0);
      break;
  }

  if (m_background)
    colorbar->setBgColor(color);
  else
    colorbar->setFgColor(color);
}

std::string ChangeColorCommand::onGetFriendlyName() const
{
  std::string text = "Color";

  switch (m_change) {
    case None:
      return text;
    case IncrementIndex:
      text += ": Increment";
      break;
    case DecrementIndex:
      text += ": Decrement";
      break;
  }

  if (m_background)
    text += " Background Index";
  else
    text += " Foreground Index";

  return text;
}

Command* CommandFactory::createChangeColorCommand()
{
  return new ChangeColorCommand;
}

} // namespace app
