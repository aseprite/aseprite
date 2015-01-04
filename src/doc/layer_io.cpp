// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
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
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/subobjects_io.h"

#include <iostream>
#include <vector>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Layer data:

void write_layer(std::ostream& os, SubObjectsIO* subObjects, Layer* layer)
{
  std::string name = layer->name();

  write16(os, name.size());                            // Name length
  if (!name.empty())
    os.write(name.c_str(), name.size());               // Name

  write32(os, static_cast<int>(layer->flags())); // Flags
  write16(os, static_cast<int>(layer->type()));  // Type

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      // Number of cels
      write16(os, static_cast<LayerImage*>(layer)->getCelsCount());

      CelIterator it = static_cast<LayerImage*>(layer)->getCelBegin();
      CelIterator end = static_cast<LayerImage*>(layer)->getCelEnd();

      for (; it != end; ++it) {
        Cel* cel = *it;
        subObjects->write_cel(os, cel);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      // Number of sub-layers
      write16(os, static_cast<LayerFolder*>(layer)->getLayersCount());

      for (; it != end; ++it)
        subObjects->write_layer(os, *it);
      break;
    }

  }
}

Layer* read_layer(std::istream& is, SubObjectsIO* subObjects, Sprite* sprite)
{
  uint16_t name_length = read16(is);                // Name length
  std::vector<char> name(name_length+1);
  if (name_length > 0) {
    is.read(&name[0], name_length);                 // Name
    name[name_length] = 0;
  }
  else
    name[0] = 0;

  uint32_t flags = read32(is);                     // Flags
  uint16_t layer_type = read16(is);                // Type

  base::UniquePtr<Layer> layer;

  switch (static_cast<ObjectType>(layer_type)) {

    case ObjectType::LayerImage: {
      // Create layer
      layer.reset(new LayerImage(sprite));

      // Read cels
      int cels = read16(is);                      // Number of cels
      for (int c=0; c<cels; ++c) {
        // Read the cel
        Cel* cel = subObjects->read_cel(is);

        // Add the cel in the layer
        static_cast<LayerImage*>(layer.get())->addCel(cel);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      // Create the layer set
      layer.reset(new LayerFolder(sprite));

      // Number of sub-layers
      int layers = read16(is);
      for (int c=0; c<layers; c++) {
        Layer* child = subObjects->read_layer(is);
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

  if (layer != NULL) {
    layer->setName(&name[0]);
    layer->setFlags(static_cast<LayerFlags>(flags));
  }

  return layer.release();
}

}
