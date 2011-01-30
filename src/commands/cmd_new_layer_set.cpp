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

#include "gui/gui.h"

#include "commands/command.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// new_layer_set

class NewLayerSetCommand : public Command
{
public:
  NewLayerSetCommand();
  Command* clone() { return new NewLayerSetCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

NewLayerSetCommand::NewLayerSetCommand()
  : Command("NewLayerSet",
	    "New Layer Set",
	    CmdRecordableFlag)
{
}

bool NewLayerSetCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void NewLayerSetCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  // load the window widget
  FramePtr window(load_widget("new_layer.xml", "new_layer_set"));

  window->open_window_fg();

  if (window->get_killer() == jwidget_find_name(window, "ok")) {
    const char *name = jwidget_find_name(window, "name")->getText();
    Layer* layer = new LayerFolder(sprite);

    layer->setName(name);
    sprite->getFolder()->add_layer(layer);
    sprite->setCurrentLayer(layer);

    update_screen_for_sprite(sprite);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createNewLayerSetCommand()
{
  return new NewLayerSetCommand;
}
