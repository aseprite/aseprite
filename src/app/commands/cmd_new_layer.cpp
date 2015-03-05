// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

class NewLayerCommand : public Command {
public:
  NewLayerCommand();
  Command* clone() const override { return new NewLayerCommand(*this); }

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
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  std::string name;

  // Default name (m_name is a name specified in params)
  if (!m_name.empty())
    name = m_name;
  else
    name = get_unique_layer_name(sprite);

  // If params specify to ask the user about the name...
  if (m_ask) {
    // We open the window to ask the name
    base::UniquePtr<Window> window(app::load_widget<Window>("new_layer.xml", "new_layer"));
    Widget* name_widget = app::find_widget<Widget>(window, "name");
    name_widget->setText(name.c_str());
    name_widget->setMinSize(gfx::Size(128, 0));

    window->openWindowInForeground();

    if (window->getKiller() != window->findChild("ok"))
      return;

    name = window->findChild("name")->getText();
  }

  Layer* layer;
  {
    Transaction transaction(writer.context(), "New Layer");
    layer = document->getApi(transaction).newLayer(sprite);
    transaction.commit();
  }
  layer->setName(name);
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(1000, "Layer `%s' created", name.c_str());

  App::instance()->getMainWindow()->popTimeline();
}

static std::string get_unique_layer_name(Sprite* sprite)
{
  char buf[1024];
  std::sprintf(buf, "Layer %d", get_max_layer_num(sprite->folder())+1);
  return buf;
}

static int get_max_layer_num(Layer* layer)
{
  int max = 0;

  if (std::strncmp(layer->name().c_str(), "Layer ", 6) == 0)
    max = std::strtol(layer->name().c_str()+6, NULL, 10);

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

Command* CommandFactory::createNewLayerCommand()
{
  return new NewLayerCommand;
}

} // namespace app
