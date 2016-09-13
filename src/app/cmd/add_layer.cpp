// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_layer.h"

#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/layer_io.h"
#include "doc/subobjects_io.h"

namespace app {
namespace cmd {

using namespace doc;

AddLayer::AddLayer(Layer* folder, Layer* newLayer, Layer* afterThis)
  : m_folder(folder)
  , m_newLayer(newLayer)
  , m_afterThis(afterThis)
  , m_size(0)
{
}

void AddLayer::onExecute()
{
  Layer* folder = m_folder.layer();
  Layer* newLayer = m_newLayer.layer();
  Layer* afterThis = m_afterThis.layer();

  addLayer(folder, newLayer, afterThis);
}

void AddLayer::onUndo()
{
  Layer* folder = m_folder.layer();
  Layer* layer = m_newLayer.layer();

  write_layer(m_stream, layer);
  m_size = size_t(m_stream.tellp());

  removeLayer(folder, layer);
}

void AddLayer::onRedo()
{
  Layer* folder = m_folder.layer();
  SubObjectsFromSprite io(folder->sprite());
  Layer* newLayer = read_layer(m_stream, &io);
  Layer* afterThis = m_afterThis.layer();

  addLayer(folder, newLayer, afterThis);

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

void AddLayer::addLayer(Layer* folder, Layer* newLayer, Layer* afterThis)
{
  static_cast<LayerFolder*>(folder)->addLayer(newLayer);
  static_cast<LayerFolder*>(folder)->stackLayer(newLayer, afterThis);
  folder->incrementVersion();

  Document* doc = folder->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(folder->sprite());
  ev.layer(newLayer);
  doc->notify_observers<DocumentEvent&>(&DocumentObserver::onAddLayer, ev);
}

void AddLayer::removeLayer(Layer* folder, Layer* layer)
{
  Document* doc = folder->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocumentEvent&>(&DocumentObserver::onBeforeRemoveLayer, ev);

  static_cast<LayerFolder*>(folder)->removeLayer(layer);
  folder->incrementVersion();

  doc->notify_observers<DocumentEvent&>(&DocumentObserver::onAfterRemoveLayer, ev);

  delete layer;
}

} // namespace cmd
} // namespace app
