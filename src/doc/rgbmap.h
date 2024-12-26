// Aseprite Document Library
// Copyright (c) 2020-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_H_INCLUDED
#define DOC_RGBMAP_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/color.h"
#include "doc/fit_criteria.h"
#include "doc/rgbmap_algorithm.h"

namespace doc {

class Palette;

// Matches a RGBA value with an index in a color palette (doc::Palette).
class RgbMap {
public:
  virtual ~RgbMap() {}

  virtual void regenerateMap(const Palette* palette,
                             const int maskIndex,
                             const FitCriteria fitCriteria) = 0;

  virtual void regenerateMap(const Palette* palette, const int maskIndex) = 0;

  // Should return the best index in a palette that matches the given RGBA values.
  virtual int mapColor(const color_t rgba) const = 0;

  virtual int maskIndex() const = 0;

  virtual RgbMapAlgorithm rgbmapAlgorithm() const = 0;

  virtual int modifications() const = 0;

  // Color Best Fit Criteria used to generate the rgbmap
  virtual FitCriteria fitCriteria() const = 0;
  virtual void fitCriteria(const FitCriteria fitCriteria) = 0;

  int mapColor(const int r, const int g, const int b, const int a) const
  {
    ASSERT(r >= 0 && r < 256);
    ASSERT(g >= 0 && g < 256);
    ASSERT(b >= 0 && b < 256);
    ASSERT(a >= 0 && a < 256);
    return mapColor(rgba(r, g, b, a));
  }
};

} // namespace doc

#endif
