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
#include "undoable.h"
#include "widgets/colbar.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// flatten_layers

class FlattenLayersCommand : public Command
{
public:
  FlattenLayersCommand();
  Command* clone() { return new FlattenLayersCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

FlattenLayersCommand::FlattenLayersCommand()
  : Command("flatten_layers",
	    "Flatten Layers",
	    CmdUIOnlyFlag)
{
}

bool FlattenLayersCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void FlattenLayersCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int bgcolor = get_color_for_image(sprite->imgtype,
				    colorbar_get_bg_color(app_get_colorbar()));
  {
    Undoable undoable(sprite, "Flatten Layers");
    undoable.flatten_layers(bgcolor);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_flatten_layers_command()
{
  return new FlattenLayersCommand;
}
