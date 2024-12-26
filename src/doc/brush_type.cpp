// Aseprite Document Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/brush_type.h"

namespace doc {

std::string brush_type_to_string_id(BrushType brushType)
{
  switch (brushType) {
    case kCircleBrushType: return "circle";
    case kSquareBrushType: return "square";
    case kLineBrushType:   return "line";
    case kImageBrushType:  return "image";
  }
  return "unknown";
}

BrushType string_id_to_brush_type(const std::string& s)
{
  if (s == "circle")
    return kCircleBrushType;
  if (s == "square")
    return kSquareBrushType;
  if (s == "line")
    return kLineBrushType;
  if (s == "image")
    return kImageBrushType;
  return kFirstBrushType;
}

} // namespace doc
