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
#include "app.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

class UndoCommand : public Command
{
public:
  UndoCommand();
  Command* clone() { return new UndoCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

UndoCommand::UndoCommand()
  : Command("undo",
	    "Undo",
	    CmdUIOnlyFlag)
{
}

bool UndoCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    undo_can_undo(sprite->undo);
}

void UndoCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  app_get_statusbar()
    ->showTip(1000, _("Undid %s"),
	      undo_get_next_undo_label(sprite->undo));

  undo_do_undo(sprite->undo);
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_undo_command()
{
  return new UndoCommand;
}
