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

#include "base/bind.h"
#include "gui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// layer_properties

class LayerPropertiesCommand : public Command
{
public:
  LayerPropertiesCommand();
  Command* clone() { return new LayerPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command("LayerProperties",
	    "Layer Properties",
	    CmdRecordableFlag)
{
}

bool LayerPropertiesCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->getCurrentLayer() != NULL;
}

void LayerPropertiesCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  Layer* layer = sprite->getCurrentLayer();

  FramePtr window(new Frame(false, "Layer Properties"));
  Box* box1 = new Box(JI_VERTICAL);
  Box* box2 = new Box(JI_HORIZONTAL);
  Box* box3 = new Box(JI_HORIZONTAL + JI_HOMOGENEOUS);
  Widget* label_name = new Label("Name:");
  Entry* entry_name = new Entry(256, layer->getName().c_str());
  Button* button_ok = new Button("&OK");
  Button* button_cancel = new Button("&Cancel");

  button_ok->Click.connect(Bind<void>(&Frame::closeWindow, window.get(), button_ok));
  button_cancel->Click.connect(Bind<void>(&Frame::closeWindow, window.get(), button_cancel));

  jwidget_set_min_size(entry_name, 128, 0);
  jwidget_expansive(entry_name, true);

  jwidget_add_child(box2, label_name);
  jwidget_add_child(box2, entry_name);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box3, button_ok);
  jwidget_add_child(box3, button_cancel);
  jwidget_add_child(box1, box3);
  jwidget_add_child(window, box1);

  jwidget_magnetic(entry_name, true);
  jwidget_magnetic(button_ok, true);

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    layer->setName(entry_name->getText());

    update_screen_for_sprite(sprite);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}
