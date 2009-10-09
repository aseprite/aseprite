/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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
  bool enabled(Context* context);
  void execute(Context* context);
};

PasteCommand::PasteCommand()
  : Command("paste",
	    "Paste",
	    CmdUIOnlyFlag)
{
}

bool PasteCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    clipboard::can_paste();
}

void PasteCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  if (undo_is_enabled(sprite->undo))
    undo_set_label(sprite->undo, "Paste");

  clipboard::paste(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_paste_command()
{
  return new PasteCommand;
}
