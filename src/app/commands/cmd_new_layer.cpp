// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/move_layer.h"
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

#include "new_layer.xml.h"

#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

class NewLayerCommand : public Command {
public:
  NewLayerCommand();
  Command* clone() const override { return new NewLayerCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string getUniqueLayerName(const Sprite* sprite) const;
  int getMaxLayerNum(const Layer* layer) const;
  const char* layerPrefix() const;

  bool m_ask;
  bool m_top;
  bool m_group;
  std::string m_name;
};

NewLayerCommand::NewLayerCommand()
  : Command("NewLayer",
            "New Layer",
            CmdRecordableFlag)
{
  m_ask = false;
  m_top = false;
  m_group = false;
  m_name = "";
}

void NewLayerCommand::onLoadParams(const Params& params)
{
  m_ask = (params.get("ask") == "true");
  m_top = (params.get("top") == "true");
  m_group = (params.get("group") == "true");
  m_name = params.get("name");
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
    name = getUniqueLayerName(sprite);

  // If params specify to ask the user about the name...
  if (m_ask) {
    // We open the window to ask the name
    app::gen::NewLayer window;
    window.name()->setText(name.c_str());
    window.name()->setMinSize(gfx::Size(128, 0));
    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    name = window.name()->text();
  }

  LayerGroup* parent = sprite->root();
  Layer* activeLayer = writer.layer();
  SelectedLayers selLayers = writer.site()->selectedLayers();
  if (activeLayer) {
    if (activeLayer->isGroup() &&
        activeLayer->isExpanded() &&
        !m_group) {
      parent = static_cast<LayerGroup*>(activeLayer);
      activeLayer = nullptr;
    }
    else {
      parent = activeLayer->parent();
    }
  }

  Layer* layer;
  {
    Transaction transaction(
      writer.context(),
      std::string("New ") + layerPrefix());
    DocumentApi api = document->getApi(transaction);

    if (m_group)
      layer = api.newGroup(parent, name);
    else
      layer = api.newLayer(parent, name);

    ASSERT(layer->parent());

    // If "top" parameter is false, create the layer above the active
    // one.
    if (activeLayer && !m_top) {
      api.restackLayerAfter(layer,
                            activeLayer->parent(),
                            activeLayer);

      // Put all selected layers inside the group
      if (m_group && writer.site()->inTimeline()) {
        LayerGroup* commonParent = nullptr;
        layer_t sameParents = 0;
        for (Layer* l : selLayers) {
          if (!commonParent ||
              commonParent == l->parent()) {
            commonParent = l->parent();
            ++sameParents;
          }
        }

        if (sameParents == selLayers.size()) {
          for (Layer* newChild : selLayers.toLayerList()) {
            transaction.execute(
              new cmd::MoveLayer(newChild, layer,
                                 static_cast<LayerGroup*>(layer)->lastLayer()));
          }
        }
      }
    }

    transaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  StatusBar::instance()->showTip(
    1000, "%s '%s' created",
    layerPrefix(),
    name.c_str());

  App::instance()->mainWindow()->popTimeline();
}

std::string NewLayerCommand::getUniqueLayerName(const Sprite* sprite) const
{
  char buf[1024];
  std::sprintf(buf, "%s %d",
               layerPrefix(),
               getMaxLayerNum(sprite->root())+1);
  return buf;
}

int NewLayerCommand::getMaxLayerNum(const Layer* layer) const
{
  std::string prefix = layerPrefix();
  prefix += " ";

  int max = 0;
  if (std::strncmp(layer->name().c_str(), prefix.c_str(), prefix.size()) == 0)
    max = std::strtol(layer->name().c_str()+prefix.size(), NULL, 10);

  if (layer->isGroup()) {
    for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers()) {
      int tmp = getMaxLayerNum(child);
      max = MAX(tmp, max);
    }
  }

  return max;
}

const char* NewLayerCommand::layerPrefix() const
{
  return (m_group ? "Group": "Layer");
}

Command* CommandFactory::createNewLayerCommand()
{
  return new NewLayerCommand;
}

} // namespace app
