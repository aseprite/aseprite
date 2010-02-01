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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// remove_frame

class RemoveFrameCommand : public Command
{
public:
  RemoveFrameCommand();
  Command* clone() { return new RemoveFrameCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

RemoveFrameCommand::RemoveFrameCommand()
  : Command("remove_frame",
	    "Remove Frame",
	    CmdRecordableFlag)
{
}

bool RemoveFrameCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->frames > 1;
}

void RemoveFrameCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    Undoable undoable(sprite, "Remove Frame");
    undoable.remove_frame(sprite->frame);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_remove_frame_command()
{
  return new RemoveFrameCommand;
}
