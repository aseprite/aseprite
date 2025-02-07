// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_TYPE_H_INCLUDED
#define DOC_OBJECT_TYPE_H_INCLUDED
#pragma once

namespace doc {

enum class ObjectType {
  Unknown = 0,
  Image = 1,
  Palette = 2,

  // Deprecated values, we cannot re-use these indexes because
  // backup sessions use them (check readLayer() function in
  // src/app/crash/read_document.cpp).
  // RgbMap = 3,
  // Path = 4,

  Mask = 5,
  Cel = 6,
  CelData = 7,
  LayerImage = 8,
  LayerGroup = 9,
  Sprite = 10,
  Document = 11,
  Tag = 12,
  Slice = 13,

  LayerTilemap = 14,
  Tileset = 15,
  Tilesets = 16,
};

} // namespace doc

#endif // DOC_OBJECT_TYPE_H_INCLUDED
