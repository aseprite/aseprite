// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/color_bar.h"
#include "ui/base.h"

namespace app {

class SwitchColorsCommand : public Command {
public:
  SwitchColorsCommand();

protected:
  void onExecute(Context* context);
};

SwitchColorsCommand::SwitchColorsCommand()
  : Command("SwitchColors",
            "Switch Colors",
            CmdUIOnlyFlag)
{
}

void SwitchColorsCommand::onExecute(Context* context)
{
  ColorBar* colorbar = ColorBar::instance();
  app::Color fg = colorbar->getFgColor();
  app::Color bg = colorbar->getBgColor();

  colorbar->setFgColor(bg);
  colorbar->setBgColor(fg);
}

Command* CommandFactory::createSwitchColorsCommand()
{
  return new SwitchColorsCommand;
}

} // namespace app
