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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "core/app.h"
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
  bool enabled(Context* context);
  void execute(Context* context);
};

NewLayerSetCommand::NewLayerSetCommand()
  : Command("new_layer_set",
	    "New Layer Set",
	    CmdRecordableFlag)
{
}

bool NewLayerSetCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void NewLayerSetCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  // load the window widget
  JWidgetPtr window(load_widget("newlay.jid", "new_layer_set"));

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    const char *name = jwidget_get_text(jwidget_find_name(window, "name"));
    Layer* layer = new LayerFolder(sprite);

    layer->set_name(name);
    sprite->get_folder()->add_layer(layer);
    sprite_set_layer(sprite, layer);

    update_screen_for_sprite(sprite);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_new_layer_set_command()
{
  return new NewLayerSetCommand;
}
