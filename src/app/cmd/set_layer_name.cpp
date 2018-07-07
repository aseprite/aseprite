// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_name.h"

#include "app/doc.h"
#include "app/doc_event.h"
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
  Doc* doc = static_cast<Doc*>(layer->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocEvent&>(&DocObserver::onLayerNameChange, ev);
}

} // namespace cmd
} // namespace app
