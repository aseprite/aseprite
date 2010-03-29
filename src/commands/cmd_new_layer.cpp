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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/params.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "sprite_wrappers.h"
#include "widgets/statebar.h"

//////////////////////////////////////////////////////////////////////
// new_layer

class NewLayerCommand : public Command
{
public:
  NewLayerCommand();
  Command* clone() { return new NewLayerCommand(*this); }

protected:
  void load_params(Params* params);
  bool enabled(Context* context);
  void execute(Context* context);

private:
  bool m_ask;
  std::string m_name;
};

static std::string get_unique_layer_name(Sprite* sprite);
static int get_max_layer_num(Layer* layer);

NewLayerCommand::NewLayerCommand()
  : Command("new_layer",
	    "New Layer",
	    CmdRecordableFlag)
{
  m_ask = false;
  m_name = "";
}

void NewLayerCommand::load_params(Params* params)
{
  std::string ask = params->get("ask");
  if (ask == "true") m_ask = true;

  m_name = params->get("name");
}

bool NewLayerCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void NewLayerCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  std::string name;

  // Default name (m_name is a name specified in params)
  if (!m_name.empty())
    name = m_name;
  else
    name = get_unique_layer_name(sprite);

  // If params specify to ask the user about the name...
  if (m_ask) {
    // We open the window to ask the name
    FramePtr window(load_widget("newlay.jid", "new_layer"));
    JWidget name_widget = find_widget(window, "name");
    name_widget->setText(name.c_str());
    jwidget_set_min_size(name_widget, 128, 0);

    window->open_window_fg();

    if (window->get_killer() != jwidget_find_name(window, "ok"))
      return;

    name = jwidget_find_name(window, "name")->getText();
  }

  Layer* layer;
  {
    Undoable undoable(sprite, "New Layer");
    layer = undoable.new_layer();
    undoable.commit();
  }
  layer->set_name(name);
  update_screen_for_sprite(sprite);

  app_get_statusbar()->dirty();
  app_get_statusbar()->showTip(1000, _("Layer `%s' created"), name.c_str());
}

static std::string get_unique_layer_name(Sprite* sprite)
{
  char buf[1024];
  sprintf(buf, "Layer %d", get_max_layer_num(sprite->get_folder())+1);
  return buf;
}

static int get_max_layer_num(Layer* layer)
{
  int max = 0;

  if (strncmp(layer->get_name().c_str(), "Layer ", 6) == 0)
    max = strtol(layer->get_name().c_str()+6, NULL, 10);

  if (layer->is_folder()) {
    LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
    LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

    for (; it != end; ++it) {
      int tmp = get_max_layer_num(*it);
      max = MAX(tmp, max);
    }
  }
  
  return max;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_new_layer_command()
{
  return new NewLayerCommand;
}
