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

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// remove_cel

class RemoveCelCommand : public Command
{
public:
  RemoveCelCommand();
  Command* clone() { return new RemoveCelCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RemoveCelCommand::RemoveCelCommand()
  : Command("remove_cel",
	    "Remove Cel",
	    CmdRecordableFlag)
{
}

bool RemoveCelCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite &&
    sprite->getCurrentLayer() &&
    sprite->getCurrentLayer()->is_readable() &&
    sprite->getCurrentLayer()->is_writable() &&
    sprite->getCurrentLayer()->is_image() &&
    static_cast<const LayerImage*>(sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
}

void RemoveCelCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  Cel* cel = static_cast<LayerImage*>(sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
  {
    Undoable undoable(sprite, "Remove Cel");
    undoable.removeCel(static_cast<LayerImage*>(sprite->getCurrentLayer()), cel);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_remove_cel_command()
{
  return new RemoveCelCommand;
}
