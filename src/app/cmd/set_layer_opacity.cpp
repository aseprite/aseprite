// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_opacity.h"

#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetLayerOpacity::SetLayerOpacity(LayerImage* layer, int opacity)
  : WithLayer(layer)
  , m_oldOpacity(layer->opacity())
  , m_newOpacity(opacity)
{
}

void SetLayerOpacity::onExecute()
{
  static_cast<LayerImage*>(layer())->setOpacity(m_newOpacity);
  layer()->incrementVersion();
}

void SetLayerOpacity::onUndo()
{
  static_cast<LayerImage*>(layer())->setOpacity(m_oldOpacity);
  layer()->incrementVersion();
}

void SetLayerOpacity::onFireNotifications()
{
  Layer* layer = this->layer();
  doc::Document* doc = layer->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocumentEvent&>(&DocumentObserver::onLayerOpacityChange, ev);
}

} // namespace cmd
} // namespace app
