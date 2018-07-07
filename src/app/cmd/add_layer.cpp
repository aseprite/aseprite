// Aseprite
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
#include "doc/layer_io.h"
#include "doc/subobjects_io.h"

namespace app {
namespace cmd {

using namespace doc;

AddLayer::AddLayer(Layer* group, Layer* newLayer, Layer* afterThis)
  : m_group(group)
  , m_newLayer(newLayer)
  , m_afterThis(afterThis)
  , m_size(0)
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

  write_layer(m_stream, layer);
  m_size = size_t(m_stream.tellp());

  removeLayer(group, layer);
}

void AddLayer::onRedo()
{
  Layer* group = m_group.layer();
  SubObjectsFromSprite io(group->sprite());
  Layer* newLayer = read_layer(m_stream, &io);
  Layer* afterThis = m_afterThis.layer();

  addLayer(group, newLayer, afterThis);

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

void AddLayer::addLayer(Layer* group, Layer* newLayer, Layer* afterThis)
{
  static_cast<LayerGroup*>(group)->insertLayer(newLayer, afterThis);
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

  static_cast<LayerGroup*>(group)->removeLayer(layer);
  group->incrementVersion();
  group->sprite()->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveLayer, ev);

  delete layer;
}

} // namespace cmd
} // namespace app
