/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "commands/command.h"
#include "launcher.h"

//////////////////////////////////////////////////////////////////////
// donate

class DonateCommand : public Command
{
public:
  DonateCommand();
  Command* clone() const { return new DonateCommand(*this); }

protected:
  void execute(Context* context);
};

DonateCommand::DonateCommand()
  : Command("donate",
	    "Donate",
	    CmdUIOnlyFlag)
{
}

void DonateCommand::execute(Context* context)
{
  std::string url;
  url += WEBSITE;
  url += "donate/";

  Launcher::openUrl(url);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_donate_command()
{
  return new DonateCommand;
}
