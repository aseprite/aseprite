// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_TYPE_H_INCLUDED
#define DOC_OBJECT_TYPE_H_INCLUDED
#pragma once

namespace doc {

  enum class ObjectType {
    Unknown,
    Image,
    Palette,
    RgbMap,
    Path,
    Mask,
    Cel,
    LayerImage,
    LayerFolder,
    Stock,
    Sprite,
    Document,
  };

} // namespace doc

#endif  // DOC_OBJECT_TYPE_H_INCLUDED
