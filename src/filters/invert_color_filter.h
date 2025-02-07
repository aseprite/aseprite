// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_INVERT_COLOR_FILTER_H_INCLUDED
#define FILTERS_INVERT_COLOR_FILTER_H_INCLUDED
#pragma once

#include "filters/filter.h"

namespace filters {

class InvertColorFilter : public Filter {
public:
  // Filter implementation
  const char* getName();
  void applyToRgba(FilterManager* filterMgr);
  void applyToGrayscale(FilterManager* filterMgr);
  void applyToIndexed(FilterManager* filterMgr);
};

} // namespace filters

#endif
