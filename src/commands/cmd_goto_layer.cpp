/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008-2009  David Capello
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
  bool enabled(Context* context);
  void execute(Context* context);
};

GotoPreviousLayerCommand::GotoPreviousLayerCommand()
  : Command("goto_previous_layer",
	    "Goto Previous Layer",
	    CmdUIOnlyFlag)
{
}

bool GotoPreviousLayerCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoPreviousLayerCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int i = sprite_layer2index(sprite, sprite->layer);
  
  if (i > 0)
    i--;
  else
    i = sprite_count_layers(sprite)-1;

  sprite->layer = sprite_index2layer(sprite, i);

  update_screen_for_sprite(sprite);
  editor_update_statusbar_for_standby(current_editor);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Layer `%s' selected"),
		     sprite->layer->get_name().c_str());
}

//////////////////////////////////////////////////////////////////////
// goto_next_layer

class GotoNextLayerCommand : public Command
{
public:
  GotoNextLayerCommand();
  Command* clone() { return new GotoNextLayerCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

GotoNextLayerCommand::GotoNextLayerCommand()
  : Command("goto_next_layer",
	    "Goto Next Layer",
	    CmdUIOnlyFlag)
{
}

bool GotoNextLayerCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoNextLayerCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int i = sprite_layer2index(sprite, sprite->layer);

  if (i < sprite_count_layers(sprite)-1)
    i++;
  else
    i = 0;

  sprite->layer = sprite_index2layer(sprite, i);

  update_screen_for_sprite(sprite);
  editor_update_statusbar_for_standby(current_editor);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("Layer `%s' selected"),
		     sprite->layer->get_name().c_str());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_goto_previous_layer_command()
{
  return new GotoPreviousLayerCommand;
}

Command* CommandFactory::create_goto_next_layer_command()
{
  return new GotoNextLayerCommand;
}
