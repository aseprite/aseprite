// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_IO_H_INCLUDED
#define DOC_LAYER_IO_H_INCLUDED
#pragma once

#include "base/exception.h"

#include <iosfwd>

namespace doc {

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

} // namespace doc

#endif
