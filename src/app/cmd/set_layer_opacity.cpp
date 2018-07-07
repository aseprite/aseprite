// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_opacity.h"

#include "app/doc.h"
#include "app/doc_event.h"
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
  Doc* doc = static_cast<Doc*>(layer->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocEvent&>(&DocObserver::onLayerOpacityChange, ev);
}

} // namespace cmd
} // namespace app
