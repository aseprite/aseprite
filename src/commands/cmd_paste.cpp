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
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/clipboard.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// paste

class PasteCommand : public Command
{
public:
  PasteCommand();
  Command* clone() { return new PasteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PasteCommand::PasteCommand()
  : Command("paste",
	    "Paste",
	    CmdUIOnlyFlag)
{
}

bool PasteCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->getCurrentLayer() &&
    sprite->getCurrentLayer()->is_image() &&
    sprite->getCurrentLayer()->is_writable() &&
    clipboard::can_paste();
}

void PasteCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  clipboard::paste(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_paste_command()
{
  return new PasteCommand;
}
