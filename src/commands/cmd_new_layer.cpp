/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/gui.h"
#include "undo_transaction.h"
#include "widgets/status_bar.h"

#include <cstdio>

using namespace ui;

//////////////////////////////////////////////////////////////////////
// new_layer

class NewLayerCommand : public Command
{
public:
  NewLayerCommand();
  Command* clone() { return new NewLayerCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  bool m_ask;
  std::string m_name;
};

static std::string get_unique_layer_name(Sprite* sprite);
static int get_max_layer_num(Layer* layer);

NewLayerCommand::NewLayerCommand()
  : Command("NewLayer",
            "New Layer",
            CmdRecordableFlag)
{
  m_ask = false;
  m_name = "";
}

void NewLayerCommand::onLoadParams(Params* params)
{
  std::string ask = params->get("ask");
  if (ask == "true") m_ask = true;

  m_name = params->get("name");
}

bool NewLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewLayerCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  std::string name;

  // Default name (m_name is a name specified in params)
  if (!m_name.empty())
    name = m_name;
  else
    name = get_unique_layer_name(sprite);

  // If params specify to ask the user about the name...
  if (m_ask) {
    // We open the window to ask the name
    UniquePtr<Window> window(app::load_widget<Window>("new_layer.xml", "new_layer"));
    Widget* name_widget = app::find_widget<Widget>(window, "name");
    name_widget->setText(name.c_str());
    jwidget_set_min_size(name_widget, 128, 0);

    window->openWindowInForeground();

    if (window->getKiller() != window->findChild("ok"))
      return;

    name = window->findChild("name")->getText();
  }

  Layer* layer;
  {
    UndoTransaction undoTransaction(document, "New Layer");
    layer = undoTransaction.newLayer();
    undoTransaction.commit();
  }
  layer->setName(name);
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());
}

static std::string get_unique_layer_name(Sprite* sprite)
{
  char buf[1024];
  std::sprintf(buf, "Layer %d", get_max_layer_num(sprite->getFolder())+1);
  return buf;
}

static int get_max_layer_num(Layer* layer)
{
  int max = 0;

  if (strncmp(layer->getName().c_str(), "Layer ", 6) == 0)
    max = strtol(layer->getName().c_str()+6, NULL, 10);

  if (layer->isFolder()) {
    LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
    LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

    for (; it != end; ++it) {
      int tmp = get_max_layer_num(*it);
      max = MAX(tmp, max);
    }
  }

  return max;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createNewLayerCommand()
{
  return new NewLayerCommand;
}
