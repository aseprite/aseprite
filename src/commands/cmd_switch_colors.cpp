/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro/unicode.h>

#include "app.h"
#include "commands/command.h"
#include "gui/base.h"
#include "widgets/color_bar.h"

class SwitchColorsCommand : public Command
{
public:
  SwitchColorsCommand();

protected:
  void onExecute(Context* context);
};

SwitchColorsCommand::SwitchColorsCommand()
  : Command("SwitchColors",
            "SwitchColors",
            CmdUIOnlyFlag)
{
}

void SwitchColorsCommand::onExecute(Context* context)
{
  ColorBar* colorbar = app_get_colorbar();
  Color fg = colorbar->getFgColor();
  Color bg = colorbar->getBgColor();

  colorbar->setFgColor(bg);
  colorbar->setBgColor(fg);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createSwitchColorsCommand()
{
  return new SwitchColorsCommand;
}
