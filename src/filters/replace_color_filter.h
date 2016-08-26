// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED
#define FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED
#pragma once

#include "filters/filter.h"

namespace filters {

  class ReplaceColorFilter : public Filter {
  public:
    ReplaceColorFilter();

    void setFrom(int from);
    void setTo(int to);
    void setTolerance(int tolerance);

    int getFrom() const { return m_from; }
    int getTo() const { return m_to; }
    int getTolerance() const { return m_tolerance; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    int m_from;
    int m_to;
    int m_tolerance;
  };

} // namespace filters

#endif
