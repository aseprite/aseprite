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
#include "app.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

class RedoCommand : public Command
{
public:
  RedoCommand();
  Command* clone() { return new RedoCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

RedoCommand::RedoCommand()
  : Command("redo",
	    "Redo",
	    CmdUIOnlyFlag)
{
}

bool RedoCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    undo_can_redo(sprite->undo);
}

void RedoCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Redid %s"),
		     undo_get_next_redo_label(sprite->undo));

  undo_do_redo(sprite->undo);
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_redo_command()
{
  return new RedoCommand;
}
