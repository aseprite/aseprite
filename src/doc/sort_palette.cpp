// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/sort_palette.h"

#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"

#include <algorithm>
#include <limits>

namespace doc {

using namespace gfx;

namespace {

struct PalEntryWithIndex {
  int index;
  color_t color;
};

struct PalEntryWithIndexPredicate {
  SortPaletteBy channel;
  bool ascending;

  PalEntryWithIndexPredicate(SortPaletteBy channel, bool ascending)
    : channel(channel)
    , ascending(ascending)
  {
  }

  bool operator()(const PalEntryWithIndex& a, const PalEntryWithIndex& b)
  {
    const color_t c1 = a.color;
    const color_t c2 = b.color;

    // Handle cases where, e.g., transparent yellow
    // is visually indistinguishable from transparent
    // black. Push 0 alpha toward index 0, regardless
    // of sort order being ascending or descending.
    const uint8_t a1 = rgba_geta(c1);
    const uint8_t a2 = rgba_geta(c2);

    if (a1 == 0 && a2 == 0) {
      return true;
    }
    else if (a1 == 0) {
      return true;
    }
    else if (a2 == 0) {
      return false;
    }

    const uint8_t r1 = rgba_getr(c1);
    const uint8_t g1 = rgba_getg(c1);
    const uint8_t b1 = rgba_getb(c1);

    const uint8_t r2 = rgba_getr(c2);
    const uint8_t g2 = rgba_getg(c2);
    const uint8_t b2 = rgba_getb(c2);

    switch (channel) {
      case SortPaletteBy::RED:   return (ascending ? r1 < r2 : r2 < r1);

      case SortPaletteBy::GREEN: return (ascending ? g1 < g2 : g2 < g1);

      case SortPaletteBy::BLUE:  return (ascending ? b1 < b2 : b2 < b1);

      case SortPaletteBy::ALPHA: return (ascending ? a1 < a2 : a2 < a1);

      case SortPaletteBy::HUE:   {
        const Hsv hsv1(Rgb(r1, g1, b1));
        const Hsv hsv2(Rgb(r2, g2, b2));

        // When a color is desaturated, its hue
        // is the quotient of division by zero.
        // It is not zero, which is red.
        const int sat1 = hsv1.saturationInt();
        const int sat2 = hsv2.saturationInt();

        if (sat1 == 0 && sat2 == 0) {
          const int val1 = hsv1.valueInt();
          const int val2 = hsv2.valueInt();
          return (ascending ? val1 < val2 : val2 < val1);
        }
        else if (sat1 == 0) {
          return ascending;
        }
        else if (sat2 == 0) {
          return !ascending;
        }

        const int hue1 = hsv1.hueInt();
        const int hue2 = hsv2.hueInt();
        return (ascending ? hue1 < hue2 : hue2 < hue1);
      }

      case SortPaletteBy::SATURATION: {
        // This could be inlined with
        // (max(r, g, b) - min(r, g, b)) / max(r, g, b)
        // but (1.) there is already opportunity for
        // confusion: HSV and HSL saturation share
        // the same name but arise from different
        // calculations; (2.) HSV components can
        // almost never be compared in isolation.
        const Hsv hsv1(Rgb(r1, g1, b1));
        const Hsv hsv2(Rgb(r2, g2, b2));
        const int sat1 = hsv1.saturationInt();
        const int sat2 = hsv2.saturationInt();
        if (sat1 == sat2) {
          const int val1 = hsv1.valueInt();
          const int val2 = hsv2.valueInt();
          return (ascending ? val1 < val2 : val2 < val1);
        }
        return (ascending ? sat1 < sat2 : sat2 < sat1);
      }

      case SortPaletteBy::VALUE: {
        const Hsv hsv1(Rgb(r1, g1, b1));
        const Hsv hsv2(Rgb(r2, g2, b2));
        const int val1 = hsv1.valueInt();
        const int val2 = hsv2.valueInt();
        if (val1 == val2) {
          const int sat1 = hsv1.saturationInt();
          const int sat2 = hsv2.saturationInt();
          return (ascending ? sat1 < sat2 : sat2 < sat1);
        }
        return (ascending ? val1 < val2 : val2 < val1);
      }

      case SortPaletteBy::LUMA: {
        // Perceptual, or relative, luminance.
        // Finds the square for fast approximation
        // of 2.4 or 2.2 exponent needed to convert
        // from gamma to linear. Assumes that the
        // source for palette colors is sRGB.
        const int lum1 = rgb_luma(r1 * r1, g1 * g1, b1 * b1);
        const int lum2 = rgb_luma(r2 * r2, g2 * g2, b2 * b2);
        return (ascending ? lum1 < lum2 : lum2 < lum1);
      }

      case SortPaletteBy::LIGHTNESS: {
        // HSL Lightness
        const int mn1 = std::min(r1, std::min(g1, b1));
        const int mx1 = std::max(r1, std::max(g1, b1));
        const int light1 = (mn1 + mx1) / 2;

        const int mn2 = std::min(r2, std::min(g2, b2));
        const int mx2 = std::max(r2, std::max(g2, b2));
        const int light2 = (mn2 + mx2) / 2;

        return (ascending ? light1 < light2 : light2 < light1);
      }

      default: {
        ASSERT(false);
        return false;
      }
    }
  }
};

} // anonymous namespace

Remap sort_palette(const Palette* palette, const SortPaletteBy channel, const bool ascending)
{
  std::vector<PalEntryWithIndex> tmp(palette->size());
  for (int i = 0; i < palette->size(); ++i) {
    tmp[i].index = i;
    tmp[i].color = palette->getEntry(i);
  }

  std::stable_sort(tmp.begin(), tmp.end(), PalEntryWithIndexPredicate(channel, ascending));

  Remap remap(palette->size());
  for (int i = 0; i < palette->size(); ++i)
    remap.map(tmp[i].index, i);

  return remap;
}

} // namespace doc
