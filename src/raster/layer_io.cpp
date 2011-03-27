/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "raster/layer_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"

#include <iostream>
#include <vector>

namespace raster {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Layer data:

void write_layer(std::ostream& os, LayerSubObjectsSerializer* subObjects, Layer* layer)
{
  std::string name = layer->getName();

  write16(os, name.size());			       // Name length
  if (!name.empty())
    os.write(name.c_str(), name.size());	       // Name

  write32(os, layer->getFlags());		       // Flags
  write16(os, layer->getType());		       // Type

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE: {
      // Number of cels
      write16(os, static_cast<LayerImage*>(layer)->getCelsCount());

      CelIterator it = static_cast<LayerImage*>(layer)->getCelBegin();
      CelIterator end = static_cast<LayerImage*>(layer)->getCelEnd();

      for (; it != end; ++it) {
	Cel* cel = *it;
	subObjects->write_cel(os, cel);

	Image* image = layer->getSprite()->getStock()->getImage(cel->image);
	ASSERT(image != NULL);

	subObjects->write_image(os, image);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      // Number of sub-layers
      write16(os, static_cast<LayerFolder*>(layer)->get_layers_count());

      for (; it != end; ++it)
	subObjects->write_layer(os, *it);
      break;
    }

  }
}

Layer* read_layer(std::istream& is, LayerSubObjectsSerializer* subObjects, Sprite* sprite)
{
  uint16_t name_length = read16(is);		    // Name length
  std::vector<char> name(name_length+1);
  if (name_length > 0) {
    is.read(&name[0], name_length);		    // Name
    name[name_length] = 0;
  }
  else
    name[0] = 0;

  uint32_t flags = read32(is);			   // Flags
  uint16_t layer_type = read16(is);		   // Type

  UniquePtr<Layer> layer;

  switch (layer_type) {

    case GFXOBJ_LAYER_IMAGE: {
      // Create layer
      layer.reset(new LayerImage(sprite));

      // Read cels
      int cels = read16(is);			  // Number of cels
      for (int c=0; c<cels; ++c) {
	// Read the cel
	Cel* cel = subObjects->read_cel(is);

	// Add the cel in the layer
	static_cast<LayerImage*>(layer.get())->addCel(cel);

	// Read the cel's image
	Image* image = subObjects->read_image(is);

	sprite->getStock()->replaceImage(cel->image, image);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      // Create the layer set
      layer.reset(new LayerFolder(sprite));

      // Number of sub-layers
      int layers = read16(is);
      for (int c=0; c<layers; c++) {
	Layer* child = subObjects->read_layer(is);
	if (child)
	  static_cast<LayerFolder*>(layer.get())->add_layer(child);
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
    layer->setFlags(flags);
  }

  return layer.release();
}

}
