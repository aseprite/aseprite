// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SPRITE_SHEET_TYPE_H_INCLUDED
#define APP_SPRITE_SHEET_TYPE_H_INCLUDED
#pragma once

namespace app {

  enum class SpriteSheetType {
    None,
    Horizontal,
    Vertical,
    Rows,
    Columns,
    Packed
  };

} // namespace app

#endif
