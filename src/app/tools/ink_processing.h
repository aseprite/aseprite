// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/modules/palettes.h"
#include "doc/blend_funcs.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"
#include "filters/neighboring_pixels.h"

namespace app {
namespace tools {

using namespace gfx;
using namespace filters;

namespace {

//////////////////////////////////////////////////////////////////////
// Ink Processing
//////////////////////////////////////////////////////////////////////

template<typename Derived>
class InkProcessing {
public:
  void operator()(int x1, int y, int x2, ToolLoop* loop) {
    int x;

    // Use mask
    if (loop->useMask()) {
      Point maskOrigin(loop->getMaskOrigin());
      const Rect& maskBounds(loop->getMask()->bounds());

      if ((y < maskOrigin.y) || (y >= maskOrigin.y+maskBounds.h))
        return;

      if (x1 < maskOrigin.x)
        x1 = maskOrigin.x;

      if (x2 > maskOrigin.x+maskBounds.w-1)
        x2 = maskOrigin.x+maskBounds.w-1;

      if (Image* bitmap = loop->getMask()->bitmap()) {
        static_cast<Derived*>(this)->initIterators(loop, x1, y);

        for (x=x1; x<=x2; ++x) {
          if (bitmap->getPixel(x-maskOrigin.x, y-maskOrigin.y))
            static_cast<Derived*>(this)->processPixel(x, y);

          static_cast<Derived*>(this)->moveIterators();
        }
        return;
      }
    }

    static_cast<Derived*>(this)->initIterators(loop, x1, y);
    for (x=x1; x<=x2; ++x) {
      static_cast<Derived*>(this)->processPixel(x, y);
      static_cast<Derived*>(this)->moveIterators();
    }
  }
};

template<typename Derived, typename ImageTraits>
class SimpleInkProcessing : public InkProcessing<Derived> {
public:
  void initIterators(ToolLoop* loop, int x1, int y) {
    m_dstAddress = (typename ImageTraits::address_t)loop->getDstImage()->getPixelAddress(x1, y);
  }

  void moveIterators() {
    ++m_dstAddress;
  }

protected:
  typename ImageTraits::address_t m_dstAddress;
};

template<typename Derived, typename ImageTraits>
class DoubleInkProcessing : public InkProcessing<Derived> {
public:
  void initIterators(ToolLoop* loop, int x1, int y) {
    m_srcAddress = (typename ImageTraits::address_t)loop->getSrcImage()->getPixelAddress(x1, y);
    m_dstAddress = (typename ImageTraits::address_t)loop->getDstImage()->getPixelAddress(x1, y);
  }

  void moveIterators() {
    ++m_srcAddress;
    ++m_dstAddress;
  }

protected:
  typename ImageTraits::address_t m_srcAddress;
  typename ImageTraits::address_t m_dstAddress;
};

//////////////////////////////////////////////////////////////////////
// Copy Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class CopyInkProcessing : public SimpleInkProcessing<CopyInkProcessing<ImageTraits>, ImageTraits> {
public:
  CopyInkProcessing(ToolLoop* loop) {
    m_color = loop->getPrimaryColor();

    if (loop->getLayer()->isBackground()) {
      switch (loop->sprite()->pixelFormat()) {
        case IMAGE_RGB: m_color |= rgba_a_mask; break;
        case IMAGE_GRAYSCALE: m_color |= graya_a_mask; break;
      }
    }
  }

  void processPixel(int x, int y) {
    *SimpleInkProcessing<CopyInkProcessing<ImageTraits>, ImageTraits>::m_dstAddress = m_color;
  }

private:
  color_t m_color;
};

//////////////////////////////////////////////////////////////////////
// LockAlpha Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class LockAlphaInkProcessing : public DoubleInkProcessing<LockAlphaInkProcessing<ImageTraits>, ImageTraits> {
public:
  LockAlphaInkProcessing(ToolLoop* loop)
    : m_color(loop->getPrimaryColor())
    , m_opacity(loop->getOpacity()) {
  }

  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  const color_t m_color;
  const int m_opacity;
};

template<>
void LockAlphaInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t result = rgba_blender_normal(*m_srcAddress, m_color, m_opacity);
  *m_dstAddress = rgba(
    rgba_getr(result),
    rgba_getg(result),
    rgba_getb(result),
    rgba_geta(*m_srcAddress));
}

template<>
void LockAlphaInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t result = graya_blender_normal(*m_srcAddress, m_color, m_opacity);
  *m_dstAddress = graya(
    graya_getv(result),
    graya_geta(*m_srcAddress));
}

template<>
class LockAlphaInkProcessing<IndexedTraits> : public DoubleInkProcessing<LockAlphaInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  LockAlphaInkProcessing(ToolLoop* loop)
    : m_palette(get_current_palette())
    , m_rgbmap(loop->getRgbMap())
    , m_color(m_palette->getEntry(loop->getPrimaryColor()))
    , m_opacity(loop->getOpacity())
    , m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()) {
  }

  void processPixel(int x, int y) {
    color_t c = *m_srcAddress;
    if (c == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    color_t result = rgba_blender_normal(c, m_color, m_opacity);
    *m_dstAddress = m_palette->findBestfit(
      rgba_getr(result),
      rgba_getg(result),
      rgba_getb(result),
      rgba_geta(c), m_maskIndex);
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const color_t m_color;
  const int m_opacity;
  const int m_maskIndex;
};

//////////////////////////////////////////////////////////////////////
// Transparent Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class TransparentInkProcessing : public DoubleInkProcessing<TransparentInkProcessing<ImageTraits>, ImageTraits> {
public:
  TransparentInkProcessing(ToolLoop* loop) {
    m_color = loop->getPrimaryColor();
    m_opacity = loop->getOpacity();
  }

  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  color_t m_color;
  int m_opacity;
};

template<>
void TransparentInkProcessing<RgbTraits>::processPixel(int x, int y) {
  *m_dstAddress = rgba_blender_normal(*m_srcAddress, m_color, m_opacity);
}

template<>
void TransparentInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blender_normal(*m_srcAddress, m_color, m_opacity);
}

template<>
class TransparentInkProcessing<IndexedTraits> : public DoubleInkProcessing<TransparentInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  TransparentInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_color(m_palette->getEntry(loop->getPrimaryColor())),
    m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()) {
  }

  void processPixel(int x, int y) {
    color_t c = *m_srcAddress;
    if (c == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    c = rgba_blender_normal(c, m_color, m_opacity);
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const int m_opacity;
  const color_t m_color;
  const int m_maskIndex;
};

//////////////////////////////////////////////////////////////////////
// Merge Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class MergeInkProcessing : public DoubleInkProcessing<MergeInkProcessing<ImageTraits>, ImageTraits> {
public:
  MergeInkProcessing(ToolLoop* loop) {
    m_color = loop->getPrimaryColor();
    m_opacity = loop->getOpacity();
  }

  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  color_t m_color;
  int m_opacity;
};

template<>
void MergeInkProcessing<RgbTraits>::processPixel(int x, int y) {
  *m_dstAddress = rgba_blender_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
void MergeInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blender_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
class MergeInkProcessing<IndexedTraits> : public DoubleInkProcessing<MergeInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  MergeInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()),
    m_color(loop->getPrimaryColor() == m_maskIndex ?
            (m_palette->getEntry(loop->getPrimaryColor()) & rgba_rgb_mask):
            (m_palette->getEntry(loop->getPrimaryColor()))) {
  }

  void processPixel(int x, int y) {
    color_t c = *m_srcAddress;
    if (c == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    c = rgba_blender_merge(c, m_color, m_opacity);
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const int m_opacity;
  const int m_maskIndex;
  const color_t m_color;
};

//////////////////////////////////////////////////////////////////////
// Blur Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BlurInkProcessing : public DoubleInkProcessing<BlurInkProcessing<ImageTraits>, ImageTraits> {
public:
  BlurInkProcessing(ToolLoop* loop) {
  }
  void processPixel(int x, int y) {
    // Do nothing (it's specialized for each case)
  }
};

template<>
class BlurInkProcessing<RgbTraits> : public DoubleInkProcessing<BlurInkProcessing<RgbTraits>, RgbTraits> {
public:
  BlurInkProcessing(ToolLoop* loop) :
    m_opacity(loop->getOpacity()),
    m_tiledMode(loop->getTiledMode()),
    m_srcImage(loop->getSrcImage()) {
  }

  void processPixel(int x, int y) {
    m_area.reset();
    get_neighboring_pixels<RgbTraits>(m_srcImage, x, y, 3, 3, 1, 1, m_tiledMode, m_area);

    if (m_area.count > 0) {
      m_area.r /= m_area.count;
      m_area.g /= m_area.count;
      m_area.b /= m_area.count;
      m_area.a /= 9;

      RgbTraits::pixel_t c = *m_srcAddress;
      m_area.r = rgba_getr(c) + (m_area.r-rgba_getr(c)) * m_opacity / 255;
      m_area.g = rgba_getg(c) + (m_area.g-rgba_getg(c)) * m_opacity / 255;
      m_area.b = rgba_getb(c) + (m_area.b-rgba_getb(c)) * m_opacity / 255;
      m_area.a = rgba_geta(c) + (m_area.a-rgba_geta(c)) * m_opacity / 255;

      *m_dstAddress = rgba(m_area.r, m_area.g, m_area.b, m_area.a);
    }
    else {
      *m_dstAddress = *m_srcAddress;
    }
  }

private:
  struct GetPixelsDelegate {
    int count, r, g, b, a;

    void reset() { count = r = g = b = a = 0; }

    void operator()(RgbTraits::pixel_t color)
    {
      if (rgba_geta(color) != 0) {
        r += rgba_getr(color);
        g += rgba_getg(color);
        b += rgba_getb(color);
        a += rgba_geta(color);
        ++count;
      }
    }
  };

  int m_opacity;
  TiledMode m_tiledMode;
  const Image* m_srcImage;
  GetPixelsDelegate m_area;
};

template<>
class BlurInkProcessing<GrayscaleTraits> : public DoubleInkProcessing<BlurInkProcessing<GrayscaleTraits>, GrayscaleTraits> {
public:
  BlurInkProcessing(ToolLoop* loop) :
    m_opacity(loop->getOpacity()),
    m_tiledMode(loop->getTiledMode()),
    m_srcImage(loop->getSrcImage()) {
  }

  void processPixel(int x, int y) {
    m_area.reset();
    get_neighboring_pixels<GrayscaleTraits>(m_srcImage, x, y, 3, 3, 1, 1, m_tiledMode, m_area);

    if (m_area.count > 0) {
      m_area.v /= m_area.count;
      m_area.a /= 9;

      GrayscaleTraits::pixel_t c = *m_srcAddress;
      m_area.v = graya_getv(c) + (m_area.v-graya_getv(c)) * m_opacity / 255;
      m_area.a = graya_geta(c) + (m_area.a-graya_geta(c)) * m_opacity / 255;

      *m_dstAddress = graya(m_area.v, m_area.a);
    }
    else {
      *m_dstAddress = *m_srcAddress;
    }
  }

private:
  struct GetPixelsDelegate {
    int count, v, a;

    void reset() { count = v = a = 0; }

    void operator()(GrayscaleTraits::pixel_t color)
    {
      if (graya_geta(color) > 0) {
        v += graya_getv(color);
        a += graya_geta(color);
        ++count;
      }
    }
  };

  int m_opacity;
  TiledMode m_tiledMode;
  const Image* m_srcImage;
  GetPixelsDelegate m_area;
};

template<>
class BlurInkProcessing<IndexedTraits> : public DoubleInkProcessing<BlurInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  BlurInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_tiledMode(loop->getTiledMode()),
    m_srcImage(loop->getSrcImage()),
    m_area(get_current_palette(),
           loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()) {
  }

  void processPixel(int x, int y) {
    m_area.reset();
    get_neighboring_pixels<IndexedTraits>(m_srcImage, x, y, 3, 3, 1, 1, m_tiledMode, m_area);

    if (m_area.count > 0) {
      m_area.r /= m_area.count;
      m_area.g /= m_area.count;
      m_area.b /= m_area.count;
      m_area.a /= 9;

      uint32_t color32 = m_palette->getEntry(*m_srcAddress);
      m_area.r = rgba_getr(color32) + (m_area.r-rgba_getr(color32)) * m_opacity / 255;
      m_area.g = rgba_getg(color32) + (m_area.g-rgba_getg(color32)) * m_opacity / 255;
      m_area.b = rgba_getb(color32) + (m_area.b-rgba_getb(color32)) * m_opacity / 255;
      m_area.a = rgba_geta(color32) + (m_area.a-rgba_geta(color32)) * m_opacity / 255;

      *m_dstAddress = m_rgbmap->mapColor(m_area.r, m_area.g, m_area.b, m_area.a);
    }
    else {
      *m_dstAddress = *m_srcAddress;
    }
  }

private:
  struct GetPixelsDelegate {
    const Palette* pal;
    int count, r, g, b, a;
    color_t maskColor;

    GetPixelsDelegate(const Palette* pal,
                      color_t maskColor)
      : pal(pal), maskColor(maskColor) { }

    void reset() { count = r = g = b = a = 0; }

    void operator()(IndexedTraits::pixel_t color)
    {
      if (color == maskColor)
        return;

      uint32_t color32 = pal->getEntry(color);
      if (rgba_geta(color32) > 0) {
        r += rgba_getr(color32);
        g += rgba_getg(color32);
        b += rgba_getb(color32);
        a += rgba_geta(color32);
        ++count;
      }
    }
  };

  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  int m_opacity;
  TiledMode m_tiledMode;
  const Image* m_srcImage;
  GetPixelsDelegate m_area;
};

//////////////////////////////////////////////////////////////////////
// Replace Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class ReplaceInkProcessing : public DoubleInkProcessing<ReplaceInkProcessing<ImageTraits>, ImageTraits> {
public:
  ReplaceInkProcessing(ToolLoop* loop) {
    m_color1 = loop->getPrimaryColor();
    m_color2 = loop->getSecondaryColor();
    m_opacity = loop->getOpacity();
  }

  void processPixel(int x, int y) {
    // Do nothing (it's specialized for each case)
  }

private:
  color_t m_color1;
  color_t m_color2;
  int m_opacity;
};

template<>
void ReplaceInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t src = (*m_srcAddress);

  // Colors (m_srcAddress and m_color1) match if:
  // * They are both completelly transparent (alpha == 0)
  // * Or they are not transparent and the RGB values are the same
  if ((rgba_geta(src) == 0 && rgba_geta(m_color1) == 0) ||
      (rgba_geta(src) > 0 && rgba_geta(m_color1) > 0 &&
       ((src & rgba_rgb_mask) == (m_color1 & rgba_rgb_mask)))) {
    *m_dstAddress = rgba_blender_merge(src, m_color2, m_opacity);
  }
}

template<>
void ReplaceInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t src = (*m_srcAddress);

  if ((graya_geta(src) == 0 && graya_geta(m_color1) == 0) ||
      (graya_geta(src) > 0 && graya_geta(m_color1) > 0 &&
       ((src & graya_v_mask) == (m_color1 & graya_v_mask)))) {
    *m_dstAddress = graya_blender_merge(src, m_color2, m_opacity);
  }
}

template<>
class ReplaceInkProcessing<IndexedTraits> : public DoubleInkProcessing<ReplaceInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  ReplaceInkProcessing(ToolLoop* loop) {
    m_palette = get_current_palette();
    m_rgbmap = loop->getRgbMap();
    m_color1 = loop->getPrimaryColor();
    m_color2 = loop->getSecondaryColor();
    m_opacity = loop->getOpacity();
    if (m_opacity < 255)
      m_color2 = m_palette->getEntry(m_color2);
  }

  void processPixel(int x, int y) {
    if (*m_srcAddress == m_color1) {
      if (m_opacity == 255)
        *m_dstAddress = m_color2;
      else {
        color_t c = rgba_blender_normal(
          m_palette->getEntry(*m_srcAddress), m_color2, m_opacity);

        *m_dstAddress = m_rgbmap->mapColor(
          rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c));
      }
    }
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  color_t m_color1;
  color_t m_color2;
  int m_opacity;
};

//////////////////////////////////////////////////////////////////////
// Jumble Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class JumbleInkProcessing : public DoubleInkProcessing<JumbleInkProcessing<ImageTraits>, ImageTraits> {
public:
  JumbleInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_speed(loop->getSpeed() / 4),
    m_opacity(loop->getOpacity()),
    m_tiledMode(loop->getTiledMode()),
    m_srcImage(loop->getSrcImage()),
    m_srcImageWidth(m_srcImage->width()),
    m_srcImageHeight(m_srcImage->height()) {
  }

  void processPixel(int x, int y) {
    // Do nothing (it's specialized for each case)
  }

private:
  void pickColorFromArea(int x, int y) {
    int u = x + (rand() % 3)-1 - m_speed.x;
    int v = y + (rand() % 3)-1 - m_speed.y;

    if (int(m_tiledMode) & int(TiledMode::X_AXIS)) {
      if (u < 0)
        u = m_srcImageWidth - (-(u+1) % m_srcImageWidth) - 1;
      else if (u >= m_srcImageWidth)
        u %= m_srcImageWidth;
    }
    else {
      u = MID(0, u, m_srcImageWidth-1);
    }

    if (int(m_tiledMode) & int(TiledMode::Y_AXIS)) {
      if (v < 0)
        v = m_srcImageHeight - (-(v+1) % m_srcImageHeight) - 1;
      else if (v >= m_srcImageHeight)
        v %= m_srcImageHeight;
    }
    else {
      v = MID(0, v, m_srcImageHeight-1);
    }
    m_color = get_pixel(m_srcImage, u, v);
  }

  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  Point m_speed;
  int m_opacity;
  TiledMode m_tiledMode;
  const Image* m_srcImage;
  int m_srcImageWidth;
  int m_srcImageHeight;
  color_t m_color;
};

template<>
void JumbleInkProcessing<RgbTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);
  *m_dstAddress = rgba_blender_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
void JumbleInkProcessing<GrayscaleTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);
  *m_dstAddress = graya_blender_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
void JumbleInkProcessing<IndexedTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);

  color_t tc = (m_color != 0 ? m_palette->getEntry(m_color): 0);
  color_t c = rgba_blender_merge(*m_srcAddress != 0 ?
                                 m_palette->getEntry(*m_srcAddress): 0,
                                 tc, m_opacity);

  if (rgba_geta(c) >= 128)
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
  else
    *m_dstAddress = 0;
}

//////////////////////////////////////////////////////////////////////
// Shading Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class ShadingInkProcessing : public DoubleInkProcessing<ShadingInkProcessing<ImageTraits>, ImageTraits> {
public:
  ShadingInkProcessing(ToolLoop* loop) {
  }
  void processPixel(int x, int y) {
    // Do nothing (it's specialized for each case)
  }
};

template<>
class ShadingInkProcessing<RgbTraits> : public DoubleInkProcessing<ShadingInkProcessing<RgbTraits>, RgbTraits> {
public:
  ShadingInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_remap(loop->getShadingRemap()),
    m_left(loop->getMouseButton() == ToolLoop::Left) {
  }

  void processPixel(int x, int y) {
    color_t src = *m_srcAddress;
    int i = m_rgbmap->mapColor(rgba_getr(src),
                               rgba_getg(src),
                               rgba_getb(src),
                               rgba_geta(src));

    // The color must be an exact match
    if (src != m_palette->getEntry(i)) {
      *m_dstAddress = src;
      return;
    }

    if (m_remap) {
      i = (*m_remap)[i];
    }
    else {
      if (m_left) {
        --i;
        if (i < 0)
          i = m_palette->size()-1;
      }
      else {
        ++i;
        if (i >= m_palette->size())
          i = 0;
      }
    }

    *m_dstAddress = m_palette->getEntry(i);
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const Remap* m_remap;
  bool m_left;
};

template<>
class ShadingInkProcessing<GrayscaleTraits> : public DoubleInkProcessing<ShadingInkProcessing<GrayscaleTraits>, GrayscaleTraits> {
public:
  ShadingInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_remap(loop->getShadingRemap()),
    m_left(loop->getMouseButton() == ToolLoop::Left) {
  }

  void processPixel(int x, int y) {
    color_t src = *m_srcAddress;
    int i = graya_getv(src);

    if (m_remap) {
      i = (*m_remap)[i];
    }
    else {
      if (m_left) {
        --i;
        if (i < 0)
          i = 255;
      }
      else {
        ++i;
        if (i > 255)
          i = 0;
      }
    }

    *m_dstAddress = graya(i, graya_geta(src));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const Remap* m_remap;
  bool m_left;
};

template<>
class ShadingInkProcessing<IndexedTraits> : public DoubleInkProcessing<ShadingInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  ShadingInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_remap(loop->getShadingRemap()),
    m_left(loop->getMouseButton() == ToolLoop::Left) {
  }

  void processPixel(int x, int y) {
    color_t src = *m_srcAddress;
    int i = src;

    if (m_remap) {
      i = (*m_remap)[i];
    }
    else {
      if (m_left) {
        --i;
        if (i < 0)
          i = m_palette->size()-1;
      }
      else {
        ++i;
        if (i >= m_palette->size())
          i = 0;
      }
    }

    *m_dstAddress = i;
  }

private:
  const Palette* m_palette;
  const Remap* m_remap;
  bool m_left;
};

//////////////////////////////////////////////////////////////////////
// Xor Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class XorInkProcessing : public DoubleInkProcessing<XorInkProcessing<ImageTraits>, ImageTraits> {
public:
  XorInkProcessing(ToolLoop* loop) {
    m_color = loop->getPrimaryColor();
  }
  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  color_t m_color;
};

template<>
void XorInkProcessing<RgbTraits>::processPixel(int x, int y) {
  *m_dstAddress = rgba_blender_neg_bw(*m_srcAddress, m_color, 255);
}

template<>
void XorInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blender_neg_bw(*m_srcAddress, m_color, 255);
}

template<>
class XorInkProcessing<IndexedTraits> : public DoubleInkProcessing<XorInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  XorInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_color(m_palette->getEntry(loop->getPrimaryColor())) {
  }

  void processPixel(int x, int y) {
    color_t c = rgba_blender_neg_bw(m_palette->getEntry(*m_srcAddress), m_color, 255);
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  color_t m_color;
};

//////////////////////////////////////////////////////////////////////
// Brush Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushInkProcessing : public DoubleInkProcessing<BrushInkProcessing<ImageTraits>, ImageTraits> {
public:
  BrushInkProcessing(ToolLoop* loop) {
    m_fgColor = loop->getPrimaryColor();
    m_bgColor = loop->getSecondaryColor();
    m_palette = get_current_palette();
    m_brush = loop->getBrush();
    m_brushImage = m_brush->image();
    m_opacity = loop->getOpacity();
    m_width = m_brush->bounds().w;
    m_height = m_brush->bounds().h;
    m_u = (loop->getOffset().x + m_brush->patternOrigin().x) % m_width;
    m_v = (loop->getOffset().y + m_brush->patternOrigin().y) % m_height;
  }

  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  void alignPixelPoint(int& x, int& y) {
    x = (x - m_u) % m_width;
    y = (y - m_v) % m_height;
    if (x < 0) x = m_width - ((-x) % m_width);
    if (y < 0) y = m_height - ((-y) % m_height);
  }

  color_t m_fgColor;
  color_t m_bgColor;
  const Palette* m_palette;
  const Brush* m_brush;
  const Image* m_brushImage;
  int m_opacity;
  int m_u, m_v, m_width, m_height;
};

template<>
void BrushInkProcessing<RgbTraits>::processPixel(int x, int y) {
  alignPixelPoint(x, y);

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      c = m_palette->getEntry(c);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      c = graya(m_palette->getEntry(c), graya_geta(c));
      break;
    }
    case IMAGE_BITMAP: {
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  *m_dstAddress = rgba_blender_normal(*m_srcAddress, c, m_opacity);
}

template<>
void BrushInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  alignPixelPoint(x, y);

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      c = graya(int(rgba_getr(c)) + int(rgba_getg(c)) + int(rgba_getb(c)) / 3,
                rgba_geta(c));
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      c = m_palette->getEntry(c);
      c = graya(int(rgba_getr(c)) + int(rgba_getg(c)) + int(rgba_getb(c)) / 3,
                rgba_geta(c));
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      break;
    }
    case IMAGE_BITMAP: {
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  *m_dstAddress = graya_blender_normal(*m_srcAddress, c, m_opacity);
}

template<>
void BrushInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  alignPixelPoint(x, y);

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      c = m_palette->findBestfit(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c), 0);
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      c = m_palette->findBestfit(graya_getv(c), graya_getv(c), graya_getv(c), graya_geta(c), 0);
      break;
    }
    case IMAGE_BITMAP: {
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  *m_dstAddress = c;
}

//////////////////////////////////////////////////////////////////////

enum {
  INK_COPY,
  INK_LOCKALPHA,
  INK_TRANSPARENT,
  INK_MERGE,
  INK_BLUR,
  INK_REPLACE,
  INK_JUMBLE,
  INK_SHADING,
  INK_XOR,
  INK_BRUSH,
  MAX_INKS
};

template<typename InkProcessing>
void ink_processing_algo(int x1, int y, int x2, void* data)
{
  ToolLoop* loop = reinterpret_cast<ToolLoop*>(data);
  InkProcessing ink(loop);
  ink(x1, y, x2, loop);
}

AlgoHLine ink_processing[][3] =
{
#define DEFINE_INK(name)                         \
  { ink_processing_algo<name<RgbTraits> >,       \
    ink_processing_algo<name<GrayscaleTraits> >, \
    ink_processing_algo<name<IndexedTraits> > }

  DEFINE_INK(CopyInkProcessing),
  DEFINE_INK(LockAlphaInkProcessing),
  DEFINE_INK(TransparentInkProcessing),
  DEFINE_INK(MergeInkProcessing),
  DEFINE_INK(BlurInkProcessing),
  DEFINE_INK(ReplaceInkProcessing),
  DEFINE_INK(JumbleInkProcessing),
  DEFINE_INK(ShadingInkProcessing),
  DEFINE_INK(XorInkProcessing),
  DEFINE_INK(BrushInkProcessing)
};

} // anonymous namespace
} // namespace tools
} // namespace app
