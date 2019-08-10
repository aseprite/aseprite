// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/pal_ops.h"

#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"

namespace app {

void move_or_copy_palette_colors(
  doc::Palette& palette,
  doc::Palette& newPalette,
  doc::PalettePicks& picks,
  int& currentEntry,
  const int beforeIndex,
  const bool copy)
{
  if (beforeIndex >= palette.size()) {
    palette.resize(beforeIndex); // TODO is need to resize the
                                 // palette? why not "const Palette& palette"
    picks.resize(palette.size());
  }

  palette.copyColorsTo(&newPalette);
  doc::Remap remap(palette.size());

  // Copy colors
  if (copy) {
    int npicks = picks.picks();
    ASSERT(npicks >= 1);

    remap = doc::create_remap_to_expand_palette(palette.size()+npicks,
                                                npicks, beforeIndex);

    newPalette.resize(palette.size()+npicks);
    for (int i=0; i<palette.size(); ++i)
      newPalette.setEntry(remap[i], palette.getEntry(i));

    for (int i=0, j=0; i<palette.size(); ++i) {
      if (picks[i])
        newPalette.setEntry(beforeIndex + (j++), palette.getEntry(i));
    }

    for (int i=0, j=0; i<palette.size(); ++i) {
      if (picks[i]) {
        if (currentEntry == i) {
          currentEntry = beforeIndex + j;
          break;
        }
        ++j;
      }
    }

    for (int i=0; i<palette.size(); ++i)
      picks[i] = (i >= beforeIndex && i < beforeIndex + npicks);
  }
  // Move colors
  else {
    remap = doc::create_remap_to_move_picks(picks, beforeIndex);

    auto oldPicks = picks;
    for (int i=0; i<palette.size(); ++i) {
      newPalette.setEntry(remap[i], palette.getEntry(i));
      picks[remap[i]] = oldPicks[i];
    }

    currentEntry = remap[currentEntry];
  }
}

} // namespace app
