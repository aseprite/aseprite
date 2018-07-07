// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/move_layer.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

MoveLayer::MoveLayer(Layer* layer,
                     Layer* newParent,
                     Layer* afterThis)
  : m_layer(layer)
  , m_oldParent(layer->parent())
  , m_oldAfterThis(layer->getPrevious())
  , m_newParent(newParent)
  , m_newAfterThis(afterThis == layer ? afterThis->getPrevious(): afterThis)
{
}

void MoveLayer::onExecute()
{
  Layer* layer = m_layer.layer();
  Layer* afterThis = m_newAfterThis.layer();
  LayerGroup* oldParent = static_cast<LayerGroup*>(m_oldParent.layer());
  LayerGroup* newParent = static_cast<LayerGroup*>(m_newParent.layer());
  ASSERT(layer);
  ASSERT(oldParent);
  ASSERT(newParent);

#if _DEBUG // Check that we are not inserting a layer inside itself
  {
    Layer* p = newParent;
    while (p) {
      ASSERT(p != layer);
      p = p->parent();
    }
  }
#endif

  oldParent->removeLayer(layer);
  newParent->insertLayer(layer, afterThis);

  if (oldParent != newParent)
    oldParent->incrementVersion();
  newParent->incrementVersion();
  layer->sprite()->incrementVersion();
}

void MoveLayer::onUndo()
{
  Layer* layer = m_layer.layer();
  Layer* afterThis = m_oldAfterThis.layer();
  LayerGroup* oldParent = static_cast<LayerGroup*>(m_oldParent.layer());
  LayerGroup* newParent = static_cast<LayerGroup*>(m_newParent.layer());
  ASSERT(layer);
  ASSERT(oldParent);
  ASSERT(newParent);

#if _DEBUG // Check that we are not inserting a layer inside itself
  {
    Layer* p = newParent;
    while (p) {
      ASSERT(p != layer);
      p = p->parent();
    }
  }
#endif

  newParent->removeLayer(layer);
  oldParent->insertLayer(layer, afterThis);

  if (oldParent != newParent)
    oldParent->incrementVersion();
  newParent->incrementVersion();
  layer->sprite()->incrementVersion();
}

void MoveLayer::onFireNotifications()
{
  Layer* layer = m_layer.layer();
  Doc* doc = static_cast<Doc*>(layer->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocEvent&>(&DocObserver::onLayerRestacked, ev);
}

} // namespace cmd
} // namespace app
