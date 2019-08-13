// Aseprite Document Library
// Copyright (c) 2018-2019 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_SPEC_H_INCLUDED
#define DOC_IMAGE_SPEC_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/color.h"
#include "doc/color_mode.h"
#include "gfx/color_space.h"
#include "gfx/rect.h"
#include "gfx/size.h"

namespace doc {

  class ImageSpec {
  public:
    ImageSpec(const ColorMode colorMode,
              const int width,
              const int height,
              const color_t maskColor = 0,
              const gfx::ColorSpacePtr& colorSpace = gfx::ColorSpace::MakeNone())
      : m_colorMode(colorMode),
        m_size(width, height),
        m_maskColor(maskColor),
        m_colorSpace(colorSpace) {
      ASSERT(width > 0);
      ASSERT(height > 0);
    }

    ColorMode colorMode() const { return m_colorMode; }
    int width() const { return m_size.w; }
    int height() const { return m_size.h; }
    const gfx::Size& size() const { return m_size; }
    gfx::Rect bounds() const { return gfx::Rect(m_size); }
    const gfx::ColorSpacePtr& colorSpace() const { return m_colorSpace; }

    // The transparent color for colored images (0 by default) or just 0 for RGBA and Grayscale
    color_t maskColor() const { return m_maskColor; }

    void setColorMode(const ColorMode colorMode) { m_colorMode = colorMode; }
    void setWidth(const int width) { m_size.w = width; }
    void setHeight(const int height) { m_size.h = height; }
    void setMaskColor(const color_t color) { m_maskColor = color; }
    void setColorSpace(const gfx::ColorSpacePtr& cs) { m_colorSpace = cs; }

    void setSize(const int width,
                 const int height) {
      m_size = gfx::Size(width, height);
    }

    void setSize(const gfx::Size& sz) {
      m_size = sz;
    }

    bool operator==(const ImageSpec& that) const {
      return (m_colorMode == that.m_colorMode &&
              m_size == that.m_size &&
              m_maskColor == that.m_maskColor &&
              ((!m_colorSpace && !that.m_colorSpace) ||
               (m_colorSpace && that.m_colorSpace &&
                m_colorSpace->nearlyEqual(*that.m_colorSpace))));
    }
    bool operator!=(const ImageSpec& that) const {
      return !operator==(that);
    }

  private:
    ColorMode m_colorMode;
    gfx::Size m_size;
    color_t m_maskColor;
    gfx::ColorSpacePtr m_colorSpace;
  };

} // namespace doc

#endif
