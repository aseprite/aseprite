// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_name.h"

#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetLayerName::SetLayerName(Layer* layer, const std::string& name)
  : WithLayer(layer)
  , m_oldName(layer->name())
  , m_newName(name)
{
}

void SetLayerName::onExecute()
{
  layer()->setName(m_newName);
  layer()->incrementVersion();
}

void SetLayerName::onUndo()
{
  layer()->setName(m_oldName);
  layer()->incrementVersion();
}

void SetLayerName::onFireNotifications()
{
  Layer* layer = this->layer();
  doc::Document* doc = layer->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerNameChange, ev);
}

} // namespace cmd
} // namespace app
