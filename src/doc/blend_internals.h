// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_INTERNALS_H_INCLUDED
#define DOC_BLEND_INTERNALS_H_INCLUDED
#pragma once

#include "../../third_party/pixman/pixman/pixman-combine32.h"

#if !defined(MUL_UN8) || !defined(DIV_UN8)
  #error Invalid Pixman library
#endif

#include "doc/blend_funcs.h"
#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/image_traits.h"
#include "doc/palette.h"

namespace doc {

template<class DstTraits, class SrcTraits>
class BlenderHelper {
  BlendFunc m_blendFunc;
  color_t m_maskColor;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendFunc = SrcTraits::get_blender(blendMode, newBlend);
    m_maskColor = src->maskColor();
  }

  inline typename DstTraits::pixel_t operator()(typename DstTraits::pixel_t dst,
                                                typename SrcTraits::pixel_t src,
                                                int opacity)
  {
    if (src != m_maskColor)
      return (*m_blendFunc)(dst, src, opacity);
    else
      return dst;
  }
};

//////////////////////////////////////////////////////////////////////
// X -> Rgb

template<>
class BlenderHelper<RgbTraits, GrayscaleTraits> {
  BlendFunc m_blendFunc;
  color_t m_maskColor;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
    m_maskColor = src->maskColor();
  }

  inline RgbTraits::pixel_t operator()(RgbTraits::pixel_t dst,
                                       GrayscaleTraits::pixel_t src,
                                       int opacity)
  {
    if (src != m_maskColor) {
      int v = graya_getv(src);
      return (*m_blendFunc)(dst, rgba(v, v, v, graya_geta(src)), opacity);
    }
    else
      return dst;
  }
};

template<>
class BlenderHelper<RgbTraits, IndexedTraits> {
  const Palette* m_pal;
  BlendMode m_blendMode;
  BlendFunc m_blendFunc;
  color_t m_maskColor;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendMode = blendMode;
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
    m_maskColor = src->maskColor();
    m_pal = pal;
  }

  inline RgbTraits::pixel_t operator()(RgbTraits::pixel_t dst,
                                       IndexedTraits::pixel_t src,
                                       int opacity)
  {
    if (m_blendMode == BlendMode::SRC) {
      return m_pal->getEntry(src);
    }
    else {
      if (src != m_maskColor) {
        return (*m_blendFunc)(dst, m_pal->getEntry(src), opacity);
      }
      else
        return dst;
    }
  }
};

//////////////////////////////////////////////////////////////////////
// X -> Grayscale

template<>
class BlenderHelper<GrayscaleTraits, RgbTraits> {
  BlendFunc m_blendFunc;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
  }

  inline GrayscaleTraits::pixel_t operator()(GrayscaleTraits::pixel_t dst,
                                             RgbTraits::pixel_t src,
                                             int opacity)
  {
    // TODO we should be able to configure this function
    return rgba_to_graya_using_luma(src);
  }
};

//////////////////////////////////////////////////////////////////////
// X -> Indexed

template<>
class BlenderHelper<IndexedTraits, RgbTraits> {
  const Palette* m_pal;
  BlendMode m_blendMode;
  BlendFunc m_blendFunc;
  color_t m_maskColor;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendMode = blendMode;
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
    m_pal = pal;

    if (m_blendMode == BlendMode::SRC)
      m_maskColor = -1;
    else
      m_maskColor = dst->maskColor();
  }

  inline IndexedTraits::pixel_t operator()(IndexedTraits::pixel_t dst,
                                           RgbTraits::pixel_t src,
                                           int opacity)
  {
    if (dst != m_maskColor) {
      src = (*m_blendFunc)(m_pal->getEntry(dst), src, opacity);
    }
    return m_pal->findBestfit(rgba_getr(src),
                              rgba_getg(src),
                              rgba_getb(src),
                              rgba_geta(src),
                              m_maskColor);
  }
};

template<>
class BlenderHelper<IndexedTraits, IndexedTraits> {
  BlendMode m_blendMode;
  color_t m_maskColor;
  int m_paletteSize;

public:
  BlenderHelper(Image* dst,
                const Image* src,
                const Palette* pal,
                const BlendMode blendMode,
                const bool newBlend)
  {
    m_blendMode = blendMode;
    m_maskColor = src->maskColor();
    m_paletteSize = pal->size();
  }

  inline IndexedTraits::pixel_t operator()(IndexedTraits::pixel_t dst,
                                           IndexedTraits::pixel_t src,
                                           int opacity)
  {
    if (m_blendMode == BlendMode::SRC) {
      return src;
    }
    else if (m_blendMode == BlendMode::DST_OVER) {
      if (dst != m_maskColor)
        return dst;
      else
        return src;
    }
    else {
      if (src != m_maskColor && src < m_paletteSize)
        return src;
      else
        return dst;
    }
  }
};

} // namespace doc

#endif
