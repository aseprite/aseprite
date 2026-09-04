// Aseprite Document Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_IO_H_INCLUDED
#define DOC_OBJECT_IO_H_INCLUDED
#pragma once

#include "doc/cel_io.h"
#include "doc/image_io.h"
#include "doc/layer_io.h"
#include "doc/subobjects_io.h"

namespace doc {

template<typename T>
void write_object(std::ostream& os, T obj)
{
  static_assert(false, "read not available for this kind of object");
}

template<typename T>
T read_object(std::istream& is, SubObjectsIO* subObjects)
{
  static_assert(false, "read not available for this kind of object");
}

template<>
inline void write_object(std::ostream& os, Cel* cel)
{
  write_cel(os, cel);
}

template<>
inline Cel* read_object(std::istream& is, SubObjectsIO* subObjects)
{
  return read_cel(is, subObjects);
}

template<>
inline void write_object(std::ostream& os, Layer* layer)
{
  write_layer(os, layer);
}

template<>
inline void write_object(std::ostream& os, ImageRef image)
{
  write_image(os, image.get());
}

template<>
inline Layer* read_object(std::istream& is, SubObjectsIO* subObjects)
{
  return read_layer(is, subObjects);
}

template<>
inline ImageRef read_object(std::istream& is, SubObjectsIO* subObjects)
{
  return ImageRef(read_image(is));
}

} // namespace doc

#endif // DOC_OBJECT_IO_H_INCLUDED
