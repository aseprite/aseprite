// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
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
    CelData,
    LayerImage,
    LayerGroup,
    Sprite,
    Document,
    FrameTag,
    Slice,
  };

} // namespace doc

#endif  // DOC_OBJECT_TYPE_H_INCLUDED
