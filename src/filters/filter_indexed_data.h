// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_FILTER_INDEXED_DATA_H_INCLUDED
#define FILTERS_FILTER_INDEXED_DATA_H_INCLUDED
#pragma once

namespace doc {
  class Palette;
  class RgbMap;
}

namespace filters {

  // Provides a Palette and a RgbMap to help a Filter which operate
  // over an indexed image.
  class FilterIndexedData {
  public:
    virtual ~FilterIndexedData() { }
    virtual doc::Palette* getPalette() = 0;
    virtual doc::RgbMap* getRgbMap() = 0;
  };

} // namespace filters

#endif
