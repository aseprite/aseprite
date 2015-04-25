// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BRUSH_H_INCLUDED
#define DOC_BRUSH_H_INCLUDED
#pragma once

#include "doc/brush_type.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <vector>

namespace doc {

  class Brush {
  public:
    static const int kMinBrushSize = 1;
    static const int kMaxBrushSize = 64;

    Brush();
    Brush(BrushType type, int size, int angle);
    Brush(const Brush& brush);
    ~Brush();

    BrushType type() const { return m_type; }
    int size() const { return m_size; }
    int angle() const { return m_angle; }
    Image* image() { return m_image.get(); }

    const gfx::Rect& bounds() const { return m_bounds; }

    void setType(BrushType type);
    void setSize(int size);
    void setAngle(int angle);

  private:
    void clean();
    void regenerate();

    BrushType m_type;                       // Type of brush
    int m_size;                           // Size (diameter)
    int m_angle;                          // Angle in degrees 0-360
    ImageRef m_image;                     // Image of the brush
    gfx::Rect m_bounds;
  };

} // namespace doc

#endif
