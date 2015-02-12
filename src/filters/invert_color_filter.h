// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
