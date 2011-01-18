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
#include "raster/undo.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// reselect_mask

class ReselectMaskCommand : public Command
{
public:
  ReselectMaskCommand();
  Command* clone() { return new ReselectMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ReselectMaskCommand::ReselectMaskCommand()
  : Command("reselect_mask",
	    "Reselect Mask",
	    CmdRecordableFlag)
{
}

bool ReselectMaskCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->requestMask("*deselected*") != NULL;
}

void ReselectMaskCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  Mask *mask;

  // Request *deselected* mask
  mask = sprite->requestMask("*deselected*");

  /* undo */
  if (sprite->getUndo()->isEnabled()) {
    sprite->getUndo()->setLabel("Mask Reselection");
    sprite->getUndo()->undo_set_mask(sprite);
  }

  /* set the mask */
  sprite->setMask(mask);

  /* remove the *deselected* mask */
  sprite->removeMask(mask);
  mask_free(mask);

  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_reselect_mask_command()
{
  return new ReselectMaskCommand;
}
