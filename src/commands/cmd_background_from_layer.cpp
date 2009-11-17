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
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"
#include "widgets/colbar.h"

class BackgroundFromLayerCommand : public Command
{
public:
  BackgroundFromLayerCommand();
  Command* clone() const { return new BackgroundFromLayerCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

BackgroundFromLayerCommand::BackgroundFromLayerCommand()
  : Command("background_from_layer",
	    "BackgroundFromLayer",
	    CmdRecordableFlag)
{
}

bool BackgroundFromLayerCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->layer != NULL &&
    sprite_get_background_layer(sprite) == NULL &&
    sprite->layer->is_image() &&
    sprite->layer->is_readable() &&
    sprite->layer->is_writable();
}

void BackgroundFromLayerCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  // each frame of the layer to be converted as `Background' must be
  // cleared using the selected background color in the color-bar
  int bgcolor = context->get_bg_color();
  bgcolor = fixup_color_for_background(sprite->imgtype, bgcolor);

  {
    Undoable undoable(sprite, "Background from Layer");
    undoable.background_from_layer(static_cast<LayerImage*>(sprite->layer), bgcolor);
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_background_from_layer_command()
{
  return new BackgroundFromLayerCommand;
}
