// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_layer.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

AddLayer::AddLayer(Layer* group, Layer* newLayer, Layer* afterThis)
  : m_group(group)
  , m_newLayer(newLayer)
  , m_afterThis(afterThis)
{
}

void AddLayer::onExecute()
{
  Layer* group = m_group.layer();
  Layer* newLayer = m_newLayer.layer();
  Layer* afterThis = m_afterThis.layer();

  addLayer(group, newLayer, afterThis);
}

void AddLayer::onUndo()
{
  Layer* group = m_group.layer();
  Layer* layer = m_newLayer.layer();
  ASSERT(layer);

  removeLayer(group, layer);
  m_suspendedLayer.suspend(layer);
}

void AddLayer::onRedo()
{
  Layer* group = m_group.layer();
  Layer* newLayer = m_suspendedLayer.restore();
  Layer* afterThis = m_afterThis.layer();

  addLayer(group, newLayer, afterThis);
}

void AddLayer::addLayer(Layer* group, Layer* newLayer, Layer* afterThis)
{
  group->insertLayer(newLayer, afterThis);
  group->incrementVersion();
  group->sprite()->incrementVersion();

  Doc* doc = static_cast<Doc*>(group->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(group->sprite());
  ev.layer(newLayer);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddLayer, ev);
}

void AddLayer::removeLayer(Layer* group, Layer* layer)
{
  Doc* doc = static_cast<Doc*>(group->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocEvent&>(&DocObserver::onBeforeRemoveLayer, ev);

  group->removeLayer(layer);
  group->incrementVersion();
  group->sprite()->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveLayer, ev);
}

}} // namespace app::cmd
