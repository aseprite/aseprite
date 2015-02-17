// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/layer_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/cel_data.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_io.h"
#include "doc/sprite.h"
#include "doc/string_io.h"
#include "doc/subobjects_io.h"

#include <iostream>
#include <vector>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Layer data:

void write_layer(std::ostream& os, Layer* layer)
{
  std::string name = layer->name();

  write32(os, layer->id());
  write_string(os, layer->name());

  write32(os, static_cast<int>(layer->flags())); // Flags
  write16(os, static_cast<int>(layer->type()));  // Type

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      CelIterator it, begin = static_cast<LayerImage*>(layer)->getCelBegin();
      CelIterator end = static_cast<LayerImage*>(layer)->getCelEnd();

      // Images
      int images = 0;
      int celdatas = 0;
      for (it=begin; it != end; ++it) {
        Cel* cel = *it;
        if (!cel->link()) {
          ++images;
          ++celdatas;
        }
      }

      write16(os, images);
      for (it=begin; it != end; ++it) {
        Cel* cel = *it;
        if (!cel->link())
          write_image(os, cel->image());
      }

      write16(os, celdatas);
      for (it=begin; it != end; ++it) {
        Cel* cel = *it;
        if (!cel->link())
          write_celdata(os, cel->dataRef());
      }

      // Cels
      write16(os, static_cast<LayerImage*>(layer)->getCelsCount());
      for (it=begin; it != end; ++it) {
        Cel* cel = *it;
        write_cel(os, cel);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      // Number of sub-layers
      write16(os, static_cast<LayerFolder*>(layer)->getLayersCount());

      for (; it != end; ++it)
        write_layer(os, *it);
      break;
    }

  }
}

Layer* read_layer(std::istream& is, SubObjectsIO* subObjects)
{
  ObjectId id = read32(is);
  std::string name = read_string(is);
  uint32_t flags = read32(is);                     // Flags
  uint16_t layer_type = read16(is);                // Type

  base::UniquePtr<Layer> layer;

  switch (static_cast<ObjectType>(layer_type)) {

    case ObjectType::LayerImage: {
      // Create layer
      layer.reset(new LayerImage(subObjects->sprite()));

      // Read images
      int images = read16(is);  // Number of images
      for (int c=0; c<images; ++c) {
        ImageRef image(read_image(is));
        subObjects->addImageRef(image);
      }

      // Read celdatas
      int celdatas = read16(is);
      for (int c=0; c<celdatas; ++c) {
        CelDataRef celdata(read_celdata(is, subObjects));
        subObjects->addCelDataRef(celdata);
      }

      // Read cels
      int cels = read16(is);                      // Number of cels
      for (int c=0; c<cels; ++c) {
        // Read the cel
        Cel* cel = read_cel(is, subObjects);

        // Add the cel in the layer
        static_cast<LayerImage*>(layer.get())->addCel(cel);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      // Create the layer set
      layer.reset(new LayerFolder(subObjects->sprite()));

      // Number of sub-layers
      int layers = read16(is);
      for (int c=0; c<layers; c++) {
        Layer* child = read_layer(is, subObjects);
        if (child)
          static_cast<LayerFolder*>(layer.get())->addLayer(child);
        else
          break;
      }
      break;
    }

    default:
      throw InvalidLayerType("Invalid layer type found in stream");

  }

  if (layer) {
    layer->setName(name);
    layer->setFlags(static_cast<LayerFlags>(flags));
    layer->setId(id);
  }

  return layer.release();
}

}
