// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_FILTER_INDEXED_DATA_H_INCLUDED
#define FILTERS_FILTER_INDEXED_DATA_H_INCLUDED
#pragma once

namespace doc {
class Palette;
class PalettePicks;
class RgbMap;
} // namespace doc

namespace filters {

// Provides a Palette and a RgbMap to help a Filter which operate
// over an indexed image.
class FilterIndexedData {
public:
  virtual ~FilterIndexedData() {}
  virtual const doc::Palette* getPalette() const = 0;
  virtual const doc::RgbMap* getRgbMap() const = 0;

  // If a filter ask for a new palette, it means that the filter
  // will modify the palette instead of pixels.
  virtual doc::Palette* getNewPalette() = 0;

  // Get the selected palettes to be modified by a palette filter
  // (e.g. HueSaturationFilter).
  virtual doc::PalettePicks getPalettePicks() = 0;
};

} // namespace filters

#endif
