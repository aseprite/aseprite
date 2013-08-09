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

#ifndef RASTER_LAYER_IO_H_INCLUDED
#define RASTER_LAYER_IO_H_INCLUDED

#include "base/exception.h"

#include <iosfwd>

namespace raster {

  class Cel;
  class Image;
  class Layer;
  class Sprite;

  // Thrown when a invalid layer type is read from the istream.
  class InvalidLayerType : public base::Exception {
  public:
    InvalidLayerType(const char* msg) throw() : base::Exception(msg) { }
  };

  // Interface used to read sub-objects of a layer.
  class LayerSubObjectsSerializer {
  public:
    virtual ~LayerSubObjectsSerializer() { }

    // How to write cels, images, and sub-layers
    virtual void write_cel(std::ostream& os, Cel* cel) = 0;
    virtual void write_image(std::ostream& os, Image* image) = 0;
    virtual void write_layer(std::ostream& os, Layer* layer) = 0;

    // How to read cels, images, and sub-layers
    virtual Cel* read_cel(std::istream& is) = 0;
    virtual Image* read_image(std::istream& is) = 0;
    virtual Layer* read_layer(std::istream& is) = 0;
  };

  void write_layer(std::ostream& os, LayerSubObjectsSerializer* subObjects, Layer* layer);
  Layer* read_layer(std::istream& is, LayerSubObjectsSerializer* subObjects, Sprite* sprite);

} // namespace raster

#endif
