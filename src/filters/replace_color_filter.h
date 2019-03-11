// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED
#define FILTERS_REPLACE_COLOR_FILTER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "filters/filter.h"

namespace filters {

  class ReplaceColorFilter : public Filter {
  public:
    ReplaceColorFilter();

    void setFrom(const doc::color_t from);
    void setTo(const doc::color_t to);
    void setTolerance(int tolerance);

    doc::color_t getFrom() const { return m_from; }
    doc::color_t getTo() const { return m_to; }
    int getTolerance() const { return m_tolerance; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    doc::color_t m_from;
    doc::color_t m_to;
    int m_tolerance;
  };

} // namespace filters

#endif
