// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_PAL_OPS_H_INCLUDED
#define APP_UTIL_PAL_OPS_H_INCLUDED
#pragma once

namespace doc {
  class Palette;
  class PalettePicks;
}

namespace app {
  class Tx;

  void move_or_copy_palette_colors(
    doc::Palette& palette,
    doc::Palette& newPalette,
    doc::PalettePicks& picks,
    int& currentEntry,
    const int beforeIndex,
    const bool copy);

} // namespace app

#endif
