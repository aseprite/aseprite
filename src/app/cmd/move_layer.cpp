// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/move_layer.h"

#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

MoveLayer::MoveLayer(Layer* layer, Layer* afterThis)
  : m_layer(layer)
  , m_oldAfterThis(layer->getPrevious())
  , m_newAfterThis(afterThis)
{
}

void MoveLayer::onExecute()
{
  m_layer.layer()->parent()->stackLayer(
    m_layer.layer(),
    m_newAfterThis.layer());

  m_layer.layer()->parent()->incrementVersion();
}

void MoveLayer::onUndo()
{
  m_layer.layer()->parent()->stackLayer(
    m_layer.layer(),
    m_oldAfterThis.layer());

  m_layer.layer()->parent()->incrementVersion();
}

void MoveLayer::onFireNotifications()
{
  Layer* layer = m_layer.layer();
  doc::Document* doc = layer->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerRestacked, ev);
}

} // namespace cmd
} // namespace app
