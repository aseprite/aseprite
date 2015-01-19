/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_layer.h"

#include "app/cmd/object_io.h"
#include "doc/layer.h"
#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"

namespace app {
namespace cmd {

using namespace doc;

AddLayer::AddLayer(Layer* folder, Layer* newLayer, Layer* afterThis)
  : m_folder(folder)
  , m_newLayer(newLayer)
  , m_afterThis(afterThis)
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

  ObjectIO(folder->sprite()).write_layer(m_stream, layer);

  removeLayer(folder, layer);
}

void AddLayer::onRedo()
{
  Layer* folder = m_folder.layer();
  Layer* newLayer = ObjectIO(folder->sprite()).read_layer(m_stream);
  Layer* afterThis = m_afterThis.layer();

  addLayer(folder, newLayer, afterThis);

  m_stream.str(std::string());
  m_stream.clear();
}

void AddLayer::addLayer(Layer* folder, Layer* newLayer, Layer* afterThis)
{
  static_cast<LayerFolder*>(folder)->addLayer(newLayer);
  static_cast<LayerFolder*>(folder)->stackLayer(newLayer, afterThis);

  Document* doc = folder->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(folder->sprite());
  ev.layer(newLayer);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddLayer, ev);
}

void AddLayer::removeLayer(Layer* folder, Layer* layer)
{
  Document* doc = folder->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onBeforeRemoveLayer, ev);

  static_cast<LayerFolder*>(folder)->removeLayer(layer);

  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAfterRemoveLayer, ev);

  delete layer;
}

} // namespace cmd
} // namespace app
