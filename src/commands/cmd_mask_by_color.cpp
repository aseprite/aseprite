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

#include "jinete/jbase.h"

#include "commands/command.h"
#include "dialogs/maskcol.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// mask_by_color

class MaskByColorCommand : public Command
{
public:
  MaskByColorCommand();
  Command* clone() { return new MaskByColorCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

MaskByColorCommand::MaskByColorCommand()
  : Command("mask_by_color",
	    "Mask By Color",
	    CmdUIOnlyFlag)
{
}

bool MaskByColorCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void MaskByColorCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  dialogs_mask_color(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_mask_by_color_command()
{
  return new MaskByColorCommand;
}
