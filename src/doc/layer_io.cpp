// Aseprite Document Library
// Copyright (c) 2019-2025 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_io.h"

#include "base/serialization.h"
#include "doc/cel.h"
#include "doc/cel_data.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_audio.h"
#include "doc/layer_fill.h"
#include "doc/layer_fx.h"
#include "doc/layer_hitbox.h"
#include "doc/layer_io.h"
#include "doc/layer_mask.h"
#include "doc/layer_subsprite.h"
#include "doc/layer_text.h"
#include "doc/layer_tilemap.h"
#include "doc/layer_vector.h"
#include "doc/sprite.h"
#include "doc/string_io.h"
#include "doc/subobjects_io.h"
#include "doc/user_data_io.h"
#include "doc/uuid_io.h"

#include <iostream>
#include <memory>
#include <vector>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Layer data:

void write_layer(std::ostream& os, const Layer* layer)
{
  write32(os, layer->id());
  write_string(os, layer->name());
  write32(os, static_cast<int>(layer->flags())); // Flags
  write16(os, static_cast<int>(layer->type()));  // Type

  switch (layer->type()) {
    case ObjectType::LayerImage:
    case ObjectType::LayerTilemap:
    case ObjectType::LayerText:
    case ObjectType::LayerVector:
    case ObjectType::LayerAudio:
    case ObjectType::LayerHitbox:  {
      CelConstIterator it;
      const CelConstIterator begin = layer->getCelBegin();
      const CelConstIterator end = layer->getCelEnd();

      // Blend mode & opacity
      write16(os, (int)layer->blendMode());
      write8(os, layer->opacity());

      // Images
      int images = 0;
      int celdatas = 0;
      for (it = begin; it != end; ++it) {
        const Cel* cel = *it;
        if (!cel->link()) {
          if (cel->image())
            ++images;
          ++celdatas;
        }
      }

      write16(os, images);
      for (it = begin; it != end; ++it) {
        const Cel* cel = *it;
        if (!cel->link() && cel->image())
          write_image(os, cel->image());
      }

      write16(os, celdatas);
      for (it = begin; it != end; ++it) {
        const Cel* cel = *it;
        if (!cel->link())
          write_celdata(os, cel->dataRef().get());
      }

      // Cels
      write16(os, layer->getCelsCount());
      for (it = begin; it != end; ++it) {
        const Cel* cel = *it;
        write_cel(os, cel);
      }

      // Save tilemap data
      if (layer->type() == ObjectType::LayerTilemap) {
        // Tileset index
        write32(os, static_cast<const LayerTilemap*>(layer)->tilesetIndex());
      }
      break;
    }

    case ObjectType::LayerGroup: {
      // Number of sub-layers
      write16(os, static_cast<const LayerGroup*>(layer)->layersCount());

      for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers())
        write_layer(os, child);
      break;
    }
  }

  write_user_data(os, layer->userData());
  write_uuid(os, layer->uuid());
}

Layer* read_layer(std::istream& is, SubObjectsFromSprite* subObjects, const SerialFormat serial)
{
  ObjectId id = read32(is);
  std::string name = read_string(is);
  uint32_t flags = read32(is);      // Flags
  uint16_t layer_type = read16(is); // Type
  std::unique_ptr<Layer> layer;

  switch (static_cast<ObjectType>(layer_type)) {
    case ObjectType::LayerImage:
    case ObjectType::LayerTilemap:
    case ObjectType::LayerFill:
    case ObjectType::LayerMask:
    case ObjectType::LayerFx:
    case ObjectType::LayerText:
    case ObjectType::LayerVector:
    case ObjectType::LayerAudio:
    case ObjectType::LayerSubsprite:
    case ObjectType::LayerHitbox:    {
      // Create layer
      switch ((static_cast<ObjectType>(layer_type))) {
        case ObjectType::LayerImage:
          layer = std::make_unique<LayerImage>(subObjects->sprite());
          break;
        case ObjectType::LayerTilemap:
          layer = std::make_unique<LayerTilemap>(subObjects->sprite(), 0);
          break;
        case ObjectType::LayerFill:
          layer = std::make_unique<LayerFill>(subObjects->sprite());
          break;
        case ObjectType::LayerMask:
          layer = std::make_unique<LayerMask>(subObjects->sprite());
          break;
        case ObjectType::LayerFx: layer = std::make_unique<LayerFx>(subObjects->sprite()); break;
        case ObjectType::LayerText:
          layer = std::make_unique<LayerText>(subObjects->sprite());
          break;
        case ObjectType::LayerVector:
          layer = std::make_unique<LayerVector>(subObjects->sprite());
          break;
        case ObjectType::LayerAudio:
          layer = std::make_unique<LayerAudio>(subObjects->sprite());
          break;
        case ObjectType::LayerSubsprite:
          layer = std::make_unique<LayerSubsprite>(subObjects->sprite());
          break;
        case ObjectType::LayerHitbox:
          layer = std::make_unique<LayerHitbox>(subObjects->sprite());
          break;
      }

      // Blend mode & opacity
      layer->setBlendMode((BlendMode)read16(is));
      layer->setOpacity(read8(is));

      // Read images
      const int images = read16(is); // Number of images
      for (int c = 0; c < images; ++c) {
        ImageRef image(read_image(is));
        subObjects->addImageRef(image);
      }

      // Read celdatas
      const int celdatas = read16(is);
      for (int c = 0; c < celdatas; ++c) {
        CelDataRef celdata(read_celdata(is, subObjects, true, serial));
        subObjects->addCelDataRef(celdata);
      }

      // Read cels
      const int cels = read16(is); // Number of cels
      for (int c = 0; c < cels; ++c) {
        // Read the cel
        Cel* cel = read_cel(is, subObjects);
        ASSERT(cel);

        // Add the cel in the layer
        layer->addCel(cel);
      }

      // Create the layer tilemap
      if (layer->isTilemap()) {
        const doc::tileset_index tsi = read32(is); // Tileset index
        static_cast<LayerTilemap*>(layer.get())->setTilesetIndex(tsi);
      }
      break;
    }

    case ObjectType::LayerGroup: {
      // Create the layer group
      layer = std::make_unique<LayerGroup>(subObjects->sprite());

      // Number of sub-layers
      const int layers = read16(is);
      for (int c = 0; c < layers; c++) {
        Layer* child = read_layer(is, subObjects, serial);
        if (child)
          static_cast<LayerGroup*>(layer.get())->addLayer(child);
        else
          break;
      }
      break;
    }

    default: throw InvalidLayerType("Invalid layer type found in stream");
  }

  const UserData userData = read_user_data(is, serial);

  base::Uuid uuid;
  if (serial >= SerialFormat::Ver3)
    uuid = read_uuid(is);

  if (!layer)
    return nullptr;

  layer->setName(name);
  layer->setFlags(static_cast<LayerFlags>(flags));
  layer->setId(id);
  layer->setUserData(userData);
  if (serial >= SerialFormat::Ver3)
    layer->setUuid(uuid);

  return layer.release();
}

} // namespace doc
