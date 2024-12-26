// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_IO_H_INCLUDED
#define DOC_LAYER_IO_H_INCLUDED
#pragma once

#include "base/exception.h"
#include "doc/serial_format.h"

#include <iosfwd>

namespace doc {
class Layer;
class SubObjectsFromSprite;

// Thrown when a invalid layer type is read from the istream.
class InvalidLayerType : public base::Exception {
public:
  InvalidLayerType(const char* msg) throw() : base::Exception(msg) {}
};

void write_layer(std::ostream& os, const Layer* layer);
Layer* read_layer(std::istream& is,
                  SubObjectsFromSprite* subObjects,
                  SerialFormat serial = SerialFormat::LastVer);

} // namespace doc

#endif
