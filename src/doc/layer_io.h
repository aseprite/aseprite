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
  class Layer;
  class SubObjectsIO;

  // Thrown when a invalid layer type is read from the istream.
  class InvalidLayerType : public base::Exception {
  public:
    InvalidLayerType(const char* msg) throw() : base::Exception(msg) { }
  };

  void write_layer(std::ostream& os, Layer* layer);
  Layer* read_layer(std::istream& is, SubObjectsIO* subObjects);

} // namespace doc

#endif
