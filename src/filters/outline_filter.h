// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_INVERT_COLOR_FILTER_H_INCLUDED
#define FILTERS_INVERT_COLOR_FILTER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "filters/filter.h"
#include "filters/tiled_mode.h"

namespace filters {

  class OutlineFilter : public Filter {
  public:
    enum class Place { Outside, Inside };
    enum class Shape { Circle, Square, Horizontal, Vertical, NShapes };

    OutlineFilter();

    void place(const Place place) { m_place = place; }
    void shape(const Shape shape) { m_shape = shape; }
    void tiledMode(const TiledMode tiledMode) { m_tiledMode = tiledMode; }
    void color(const doc::color_t color) { m_color = color; }
    void bgColor(const doc::color_t color) { m_bgColor = color; }

    Place place() const { return m_place; }
    Shape shape() const { return m_shape; }
    TiledMode tiledMode() const { return m_tiledMode; }
    doc::color_t color() const { return m_color; }
    doc::color_t bgColor() const { return m_bgColor; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    Place m_place;
    Shape m_shape;
    TiledMode m_tiledMode;
    doc::color_t m_color;
    doc::color_t m_bgColor;
  };

} // namespace filters

#endif
