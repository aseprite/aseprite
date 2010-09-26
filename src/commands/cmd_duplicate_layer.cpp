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

#include "gui/jinete.h"

#include "commands/command.h"
#include "console.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"

static Layer *duplicate_layer(Sprite* sprite);

//////////////////////////////////////////////////////////////////////
// duplicate_layer

class DuplicateLayerCommand : public Command
{
public:
  DuplicateLayerCommand();
  Command* clone() const { return new DuplicateLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DuplicateLayerCommand::DuplicateLayerCommand()
  : Command("duplicate_layer",
	    "Duplicate Layer",
	    CmdRecordableFlag)
{
}

bool DuplicateLayerCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite && sprite->getCurrentLayer();
}

void DuplicateLayerCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  if (duplicate_layer(sprite) != NULL)
    update_screen_for_sprite(sprite);
}

static Layer* duplicate_layer(Sprite* sprite)
{
  /* open undo */
  if (sprite->getUndo()->isEnabled()) {
    sprite->getUndo()->setLabel("Layer Duplication");
    undo_open(sprite->getUndo());
  }

  Layer* layer_copy = sprite->getCurrentLayer()->duplicate_for(sprite);
  if (!layer_copy) {
    if (sprite->getUndo()->isEnabled())
      undo_close(sprite->getUndo());

    Console console;
    console.printf("Not enough memory");
    return NULL;
  }

  layer_copy->set_background(false);
  layer_copy->set_moveable(true);
  layer_copy->setName(layer_copy->getName() + " Copy");

  /* add the new layer in the sprite */
  if (sprite->getUndo()->isEnabled())
    undo_add_layer(sprite->getUndo(), sprite->getCurrentLayer()->get_parent(), layer_copy);

  sprite->getCurrentLayer()->get_parent()->add_layer(layer_copy);

  if (sprite->getUndo()->isEnabled()) {
    undo_move_layer(sprite->getUndo(), layer_copy);
    undo_set_layer(sprite->getUndo(), sprite);
    undo_close(sprite->getUndo());
  }

  sprite->getCurrentLayer()->get_parent()->move_layer(layer_copy, sprite->getCurrentLayer());
  sprite->setCurrentLayer(layer_copy);

  return layer_copy;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_duplicate_layer_command()
{
  return new DuplicateLayerCommand;
}
