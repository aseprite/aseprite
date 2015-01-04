/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "app/undoers/remove_layer.h"

#include "app/document.h"
#include "app/document_api.h"
#include "app/undoers/add_layer.h"
#include "app/undoers/object_io.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;

RemoveLayer::RemoveLayer(ObjectsContainer* objects, Document* document, Layer* layer)
  : m_documentId(objects->addObject(document))
  , m_folderId(objects->addObject(layer->parent()))
{
  Layer* after = layer->getPrevious();
  m_afterId = (after ? objects->addObject(after): 0);

  ObjectIO(objects, layer->sprite()).write_layer(m_stream, layer);
}

void RemoveLayer::dispose()
{
  delete this;
}

void RemoveLayer::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Document* document = objects->getObjectT<Document>(m_documentId);
  LayerFolder* folder = objects->getObjectT<LayerFolder>(m_folderId);
  Layer* after = (m_afterId != 0 ? objects->getObjectT<Layer>(m_afterId): NULL);

  // Read the layer from the stream
  Layer* layer =
    ObjectIO(objects, folder->sprite()).read_layer(m_stream);

  document->getApi(redoers).addLayer(folder, layer, after);
}

} // namespace undoers
} // namespace app
