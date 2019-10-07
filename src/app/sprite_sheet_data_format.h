// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SPRITE_SHEET_DATA_FORMAT_H_INCLUDED
#define APP_SPRITE_SHEET_DATA_FORMAT_H_INCLUDED
#pragma once

namespace app {

  enum class SpriteSheetDataFormat {
    JsonHash,
    JsonArray,
    Default = JsonHash
  };

} // namespace app

#endif
