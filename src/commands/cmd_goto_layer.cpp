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
#include "app.h"
#include "modules/gui.h"
#include "modules/editors.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// goto_previous_layer

class GotoPreviousLayerCommand : public Command
{
public:
  GotoPreviousLayerCommand();
  Command* clone() { return new GotoPreviousLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoPreviousLayerCommand::GotoPreviousLayerCommand()
  : Command("GotoPreviousLayer",
	    "Goto Previous Layer",
	    CmdUIOnlyFlag)
{
}

bool GotoPreviousLayerCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoPreviousLayerCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int i = sprite->layerToIndex(sprite->getCurrentLayer());
  
  if (i > 0)
    i--;
  else
    i = sprite->countLayers()-1;

  sprite->setCurrentLayer(sprite->indexToLayer(i));

  // Flash the current layer
  ASSERT(current_editor != NULL && "Current editor cannot be null when we have a current sprite");
  current_editor->flashCurrentLayer();

  app_get_statusbar()
    ->setStatusText(1000, "Layer `%s' selected",
		    sprite->getCurrentLayer()->getName().c_str());
}

//////////////////////////////////////////////////////////////////////
// goto_next_layer

class GotoNextLayerCommand : public Command
{
public:
  GotoNextLayerCommand();
  Command* clone() { return new GotoNextLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoNextLayerCommand::GotoNextLayerCommand()
  : Command("GotoNextLayer",
	    "Goto Next Layer",
	    CmdUIOnlyFlag)
{
}

bool GotoNextLayerCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoNextLayerCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int i = sprite->layerToIndex(sprite->getCurrentLayer());

  if (i < sprite->countLayers()-1)
    i++;
  else
    i = 0;

  sprite->setCurrentLayer(sprite->indexToLayer(i));

  // Flash the current layer
  ASSERT(current_editor != NULL && "Current editor cannot be null when we have a current sprite");
  current_editor->flashCurrentLayer();

  app_get_statusbar()
    ->setStatusText(1000, "Layer `%s' selected",
		    sprite->getCurrentLayer()->getName().c_str());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createGotoPreviousLayerCommand()
{
  return new GotoPreviousLayerCommand;
}

Command* CommandFactory::createGotoNextLayerCommand()
{
  return new GotoNextLayerCommand;
}
