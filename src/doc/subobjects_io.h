// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SUBOBJECTS_IO_H_INCLUDED
#define DOC_SUBOBJECTS_IO_H_INCLUDED
#pragma once

#include "doc/image_ref.h"

#include <iosfwd>

namespace doc {

  class Cel;
  class Image;
  class Layer;

  // Interface used to read sub-objects of a layer or cel.
  class SubObjectsIO {
  public:
    virtual ~SubObjectsIO() { }

    // How to write cels, images, and sub-layers
    virtual void write_cel(std::ostream& os, Cel* cel) = 0;
    virtual void write_image(std::ostream& os, Image* image) = 0;
    virtual void write_layer(std::ostream& os, Layer* layer) = 0;

    // How to read cels, images, and sub-layers
    virtual Cel* read_cel(std::istream& is) = 0;
    virtual Image* read_image(std::istream& is) = 0;
    virtual Layer* read_layer(std::istream& is) = 0;

    virtual void add_image_ref(const ImageRef& image) = 0;
    virtual ImageRef get_image_ref(ObjectId imageId) = 0;
  };

} // namespace doc

#endif
