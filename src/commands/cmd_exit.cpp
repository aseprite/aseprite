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

#include "gui/jinete.h"

#include "commands/command.h"
#include "context.h"
#include "app.h"
#include "raster/sprite.h"

//////////////////////////////////////////////////////////////////////
// exit

class ExitCommand : public Command
{
public:
  ExitCommand();
  Command* clone() { return new ExitCommand(*this); }

protected:
  void onExecute(Context* context);
};

ExitCommand::ExitCommand()
  : Command("exit",
	    "Exit",
	    CmdUIOnlyFlag)
{
}

void ExitCommand::onExecute(Context* context)
{
  Sprite *sprite = context->get_first_sprite();

  while (sprite) {
    // check if this sprite is modified
    if (sprite->isModified()) {
      if (jalert("Warning<<There are sprites with changes.<<Do you want quit anyway?||&Yes||&No") != 1) {
	return;
      }
      break;
    }
    sprite = context->get_next_sprite(sprite);
  }

  /* close the window */
  app_get_top_window()->closeWindow(NULL);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_exit_command()
{
  return new ExitCommand;
}

