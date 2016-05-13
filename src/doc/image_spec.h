// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_SPEC_H_INCLUDED
#define DOC_IMAGE_SPEC_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/color_mode.h"
#include "gfx/rect.h"
#include "gfx/size.h"

namespace doc {

  class ImageSpec {
  public:
    ImageSpec(ColorMode colorMode,
              int width,
              int height,
              int maskColor = 0)
      : m_colorMode(colorMode),
        m_width(width),
        m_height(height),
        m_maskColor(maskColor) {
      ASSERT(width > 0);
      ASSERT(height > 0);
    }

    ColorMode colorMode() const { return m_colorMode; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    gfx::Size size() const { return gfx::Size(m_width, m_height); }
    gfx::Rect bounds() const { return gfx::Rect(0, 0, m_width, m_height); }

    // The transparent color for colored images (0 by default) or just 0 for RGBA and Grayscale
    color_t maskColor() const { return m_maskColor; }

    void setColorMode(const ColorMode colorMode) { m_colorMode = colorMode; }
    void setWidth(const int width) { m_width = width; }
    void setHeight(const int height) { m_height = height; }
    void setMaskColor(const color_t color) { m_maskColor = color; }

    void setSize(const int width, const int height) {
      m_width = width;
      m_height = height;
    }

    void setSize(const gfx::Size& sz) {
      m_width = sz.w;
      m_height = sz.h;
    }

  private:
    ColorMode m_colorMode;
    int m_width;
    int m_height;
    color_t m_maskColor;
  };

} // namespace doc

#endif
