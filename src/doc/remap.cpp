// Aseprite Document Library
// Copyright (c)      2020 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/remap.h"

#include "base/base.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"

#include <algorithm>

namespace doc {

Remap create_remap_to_move_picks(const PalettePicks& picks, int beforeIndex)
{
  Remap map(picks.size());

  int selectedTotal = 0;
  int selectedBeforeIndex = 0;

  for (int i=0; i<map.size(); ++i) {
    if (picks[i]) {
      ++selectedTotal;
      if (i < beforeIndex)
        ++selectedBeforeIndex;
    }
  }

  for (int i=0, j=0, k=0; i<map.size(); ++i) {
    if (k == beforeIndex - selectedBeforeIndex)
      k += selectedTotal;

    if (picks[i]) {
      map.map(i, beforeIndex - selectedBeforeIndex + j);
      ++j;
    }
    else {
      map.map(i, k++);
    }
  }

  return map;
}

Remap create_remap_to_expand_palette(int size, int count, int beforeIndex)
{
  Remap map(size);

  int j, k = 0;
  for (int i=0; i<size; ++i) {
    if (i < beforeIndex)
      j = i;
    else if (i + count < size)
      j = i + count;
    else
      j = beforeIndex + (k++);

    map.map(i, j);
  }
  return map;
}

Remap create_remap_to_change_palette(
  const Palette* oldPalette, const Palette* newPalette,
  const int oldMaskIndex,
  const bool remapMaskIndex)
{
  Remap remap(std::max(oldPalette->size(), newPalette->size()));
  int maskIndex = oldMaskIndex;

  if (maskIndex >= 0) {
    if (remapMaskIndex &&
        oldPalette->getEntry(maskIndex) !=
        newPalette->getEntry(maskIndex)) {
      color_t maskColor = oldPalette->getEntry(maskIndex);
      int r = rgba_getr(maskColor);
      int g = rgba_getg(maskColor);
      int b = rgba_getb(maskColor);
      int a = rgba_geta(maskColor);

      // Find the new mask color
      maskIndex = newPalette->findExactMatch(r, g, b, a, -1);
      if (maskIndex >= 0)
        remap.map(oldMaskIndex, maskIndex);
    }
    else {
      remap.map(maskIndex, maskIndex);
    }
  }

  for (int i=0; i<oldPalette->size(); ++i) {
    if (i == oldMaskIndex)
      continue;

    const color_t color = oldPalette->getEntry(i);

    // If in both palettes, it's the same color, we don't need to
    // remap this entry.
    if (color == newPalette->getEntry(i)) {
      remap.map(i, i);
      continue;
    }

    int j = newPalette->findExactMatch(
      rgba_getr(color),
      rgba_getg(color),
      rgba_getb(color),
      rgba_geta(color), maskIndex);

    if (j < 0)
      j = newPalette->findBestfit(
        rgba_getr(color),
        rgba_getg(color),
        rgba_getb(color),
        rgba_geta(color), maskIndex);

    remap.map(i, j);
  }
  return remap;
}

void Remap::merge(const Remap& other)
{
  for (int i=0; i<size(); ++i) {
    m_map[i] = other[m_map[i]];
  }
}

Remap Remap::invert() const
{
  Remap inv(size());
  for (int i=0; i<size(); ++i)
    inv.map(operator[](i), i);
  return inv;
}

bool Remap::isFor8bit() const
{
  for (int i=0; i<size(); ++i) {
    // Moving entries between [0,255] range to or from [256,+inf)
    // range are invalid for 8-bit images.
    if ((i <  256 && m_map[i] >= 256) ||
        (i >= 256 && m_map[i] <  256))
      return false;
  }
  return true;
}

bool Remap::isInvertible(const PalettePicks& usedEntries) const
{
  PalettePicks picks(size());
  for (int i=0; i<size(); ++i) {
    if (!usedEntries[i])
      continue;

    int j = m_map[i];
    if (picks[j])
      return false;

    picks[j] = true;
  }
  return true;
}

} // namespace doc
