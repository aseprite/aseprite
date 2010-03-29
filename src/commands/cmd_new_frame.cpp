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

#include <assert.h>
#include "jinete/jinete.h"

#include "commands/command.h"
#include "console.h"
#include "core/color.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

#include <stdexcept>

//////////////////////////////////////////////////////////////////////
// new_frame

class NewFrameCommand : public Command
{
public:
  NewFrameCommand();
  Command* clone() { return new NewFrameCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

NewFrameCommand::NewFrameCommand()
  : Command("new_frame",
	    "New Frame",
	    CmdRecordableFlag)
{
}

bool NewFrameCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->layer != NULL &&
    sprite->layer->is_readable() &&
    sprite->layer->is_writable() &&
    sprite->layer->is_image();
}

void NewFrameCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    Undoable undoable(sprite, "New Frame");
    undoable.new_frame();
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
  app_get_statusbar()
    ->showTip(1000, _("New frame %d/%d"),
	      sprite->frame+1,
	      sprite->frames);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_new_frame_command()
{
  return new NewFrameCommand;
}
