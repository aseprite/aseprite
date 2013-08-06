/* ASEPRITE
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
#include "raster/cel.h"
#include "raster/cel_io.h"
#include "raster/image.h"
#include "raster/image_io.h"
#include "raster/layer.h"
#include "raster/layer_io.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;

class LayerSubObjectsSerializerImpl : public raster::LayerSubObjectsSerializer {
public:
  LayerSubObjectsSerializerImpl(ObjectsContainer* objects, Sprite* sprite)
    : m_objects(objects)
    , m_sprite(sprite) {
  }

  virtual ~LayerSubObjectsSerializerImpl() { }

  // How to write cels, images, and sub-layers
  void write_cel(std::ostream& os, Cel* cel) OVERRIDE {
    write_object(m_objects, os, cel, raster::write_cel);
  }

  void write_image(std::ostream& os, Image* image) OVERRIDE {
    write_object(m_objects, os, image, raster::write_image);
  }

  void write_layer(std::ostream& os, Layer* layer) OVERRIDE {
    // To write a sub-layer we use the operator() of this instance (*this)
    write_object(m_objects, os, layer, *this);
  }

  // How to read cels, images, and sub-layers
  Cel* read_cel(std::istream& is) OVERRIDE {
    return read_object<Cel>(m_objects, is, raster::read_cel);
  }

  Image* read_image(std::istream& is) OVERRIDE {
    return read_object<Image>(m_objects, is, raster::read_image);
  }

  Layer* read_layer(std::istream& is) OVERRIDE {
    // To read a sub-layer we use the operator() of this instance (*this)
    return read_object<Layer>(m_objects, is, *this);
  }

  // The following operator() calls are used in write/read_object() functions.

  void operator()(std::ostream& os, Layer* layer) {
    raster::write_layer(os, this, layer);
  }

  Layer* operator()(std::istream& is) {
    return raster::read_layer(is, this, m_sprite);
  }

private:
  ObjectsContainer* m_objects;
  Sprite* m_sprite;
};

RemoveLayer::RemoveLayer(ObjectsContainer* objects, Document* document, Layer* layer)
  : m_documentId(objects->addObject(document))
  , m_folderId(objects->addObject(layer->getParent()))
{
  Layer* after = layer->getPrevious();
  m_afterId = (after ? objects->addObject(after): 0);

  LayerSubObjectsSerializerImpl serializer(objects, layer->getSprite());
  write_object(objects, m_stream, layer, serializer);
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
  LayerSubObjectsSerializerImpl serializer(objects, folder->getSprite());
  Layer* layer = read_object<Layer>(objects, m_stream, serializer);

  document->getApi(redoers).addLayer(folder, layer, after);
}

} // namespace undoers
} // namespace app
