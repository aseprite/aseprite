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
#include "raster/mask.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"

//////////////////////////////////////////////////////////////////////
// deselect_mask

class DeselectMaskCommand : public Command
{
public:
  DeselectMaskCommand();
  Command* clone() const { return new DeselectMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DeselectMaskCommand::DeselectMaskCommand()
  : Command("deselect_mask",
	    "Deselect Mask",
	    CmdRecordableFlag)
{
}

bool DeselectMaskCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite && !sprite->getMask()->is_empty();
}

void DeselectMaskCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    Undoable undoable(sprite, "Mask Deselection");
    undoable.deselectMask();
    undoable.commit();
  }
  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_deselect_mask_command()
{
  return new DeselectMaskCommand;
}
