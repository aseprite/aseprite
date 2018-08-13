// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/modules/palettes.h"
#include "app/util/wrap_point.h"
#include "app/util/wrap_value.h"
#include "doc/blend_funcs.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"
#include "filters/neighboring_pixels.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "render/dithering.h"
#include "render/gradient.h"

namespace app {
namespace tools {

using namespace gfx;
using namespace filters;

namespace {

//////////////////////////////////////////////////////////////////////
// Ink Processing
//////////////////////////////////////////////////////////////////////

class BaseInkProcessing {
public:
  virtual ~BaseInkProcessing() { }
  virtual void processScanline(int x1, int y, int x2, ToolLoop* loop) = 0;
  virtual void prepareForStrokes(ToolLoop* loop, Strokes& strokes) { }
  virtual void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) { }
};

template<typename Derived>
class InkProcessing : public BaseInkProcessing {
public:
  void processScanline(int x1, int y, int x2, ToolLoop* loop) override {
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
    if (int(c) == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    color_t result = rgba_blender_normal(c, m_color, m_opacity);
    // TODO should we use m_rgbmap->mapColor instead?
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
    if (int(c) == m_maskIndex)
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
    m_color(int(loop->getPrimaryColor()) == m_maskIndex ?
            (m_palette->getEntry(loop->getPrimaryColor()) & rgba_rgb_mask):
            (m_palette->getEntry(loop->getPrimaryColor()))) {
  }

  void processPixel(int x, int y) {
    color_t c = *m_srcAddress;
    if (int(c) == m_maskIndex)
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

      *m_dstAddress =
        rgba_blender_normal(*m_srcAddress,
                            rgba(m_area.r, m_area.g, m_area.b, m_area.a),
                            m_opacity);
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

      *m_dstAddress =
        graya_blender_normal(*m_srcAddress,
                             graya(m_area.v, m_area.a),
                             m_opacity);
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

      color_t c =
        rgba_blender_normal(m_palette->getEntry(*m_srcAddress),
                            rgba(m_area.r, m_area.g, m_area.b, m_area.a),
                            m_opacity);

      *m_dstAddress = m_rgbmap->mapColor(
        rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c));
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
  // * They are both completely transparent (alpha == 0)
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
    gfx::Point pt(x + (rand() % 3)-1 - m_speed.x,
                  y + (rand() % 3)-1 - m_speed.y);

    pt = wrap_point(m_tiledMode,
                    gfx::Size(m_srcImageWidth,
                              m_srcImageHeight), pt);

    pt.x = MID(0, pt.x, m_srcImageWidth-1);
    pt.y = MID(0, pt.y, m_srcImageHeight-1);

    m_color = get_pixel(m_srcImage, pt.x, pt.y);
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

    // We cannot use the m_rgbmap->mapColor() function because RgbMaps
    // are created with findBestfit(), and findBestfit() limits the
    // returned indexes to [0,255] range (it's mainly used for RGBA ->
    // Indexed image conversion).
    int i = m_palette->findExactMatch(rgba_getr(src),
                                      rgba_getg(src),
                                      rgba_getb(src),
                                      rgba_geta(src),
                                      -1);

    // If we didn't find the exact match.
    if (i < 0) {
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

  // Works as the RGBA version
  void processPixel(int x, int y) {
    color_t src = *m_srcAddress;

    int i = m_palette->findExactMatch(graya_getv(src),
                                      graya_getv(src),
                                      graya_getv(src),
                                      graya_geta(src),
                                      -1);

    if (i < 0) {
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
          i = 255;
      }
      else {
        ++i;
        if (i > 255)
          i = 0;
      }
    }

    color_t rgba = m_palette->getEntry(i);
    *m_dstAddress = graya(
      int(255.0 * Hsv(Rgb(rgba_getr(rgba),
                          rgba_getg(rgba),
                          rgba_getb(rgba))).value()),
      rgba_geta(rgba));
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
// Gradient Ink
//////////////////////////////////////////////////////////////////////

static ImageBufferPtr tmpGradientBuffer; // TODO non-thread safe

class GradientRenderer {
public:
  GradientRenderer(ToolLoop* loop)
    : m_tiledMode(loop->getTiledMode())
  {
    if (!tmpGradientBuffer)
      tmpGradientBuffer.reset(new ImageBuffer(1));

    m_tmpImage.reset(
      Image::create(IMAGE_RGB,
                    loop->getDstImage()->width(),
                    loop->getDstImage()->height(),
                    tmpGradientBuffer));
    m_tmpImage->clear(0);
  }

  void renderRgbaGradient(ToolLoop* loop, Strokes& strokes,
                          // RGBA colors
                          color_t c0, color_t c1) {
    if (strokes.empty() || strokes[0].size() < 2) {
      m_tmpImage->clear(0);
      return;
    }

    const gfx::Point u = strokes[0].firstPoint();
    const gfx::Point v = strokes[0].lastPoint();

    // The image position for the gradient depends on the first user
    // click. The gradient depends on the first clicked tile.
    gfx::Point imgPos(0, 0);
    if (int(m_tiledMode) & int(TiledMode::X_AXIS)) {
      const int w = loop->sprite()->width();
      imgPos.x = u.x / w;
      imgPos.x *= w;
    }
    if (int(m_tiledMode) & int(TiledMode::Y_AXIS)) {
      const int h = loop->sprite()->height();
      imgPos.y = u.y / h;
      imgPos.y *= h;
    }

    render::render_rgba_linear_gradient(
      m_tmpImage.get(), imgPos, u, v, c0, c1,
      loop->getDitheringMatrix());
  }

protected:
  ImageRef m_tmpImage;
  RgbTraits::address_t m_tmpAddress;
  TiledMode m_tiledMode;
};

template<typename ImageTraits>
class GradientInkProcessing : public GradientRenderer,
                              public DoubleInkProcessing<GradientInkProcessing<ImageTraits>, ImageTraits> {
public:
  typedef DoubleInkProcessing<GradientInkProcessing<ImageTraits>, ImageTraits> base;

  GradientInkProcessing(ToolLoop* loop)
    : GradientRenderer(loop)
    , m_opacity(loop->getOpacity())
    , m_palette(get_current_palette())
    , m_rgbmap(loop->getRgbMap())
    , m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor())
    , m_matrix(loop->getDitheringMatrix())
  {
  }

  void processScanline(int x1, int y, int x2, ToolLoop* loop) override {
    m_tmpAddress = (RgbTraits::address_t)m_tmpImage->getPixelAddress(x1, y);
    base::processScanline(x1, y, x2, loop);
  }

  void prepareForStrokes(ToolLoop* loop, Strokes& strokes) override {
    // Do nothing
  }

  void processPixel(int x, int y) {
    // Do nothing (it's specialized for each case)
  }

private:
  const int m_opacity;
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const int m_maskIndex;
  const render::DitheringMatrix m_matrix;
};

template<>
void GradientInkProcessing<RgbTraits>::prepareForStrokes(ToolLoop* loop, Strokes& strokes)
{
  color_t c0 = loop->getPrimaryColor();
  color_t c1 = loop->getSecondaryColor();

  renderRgbaGradient(loop, strokes, c0, c1);
}

template<>
void GradientInkProcessing<RgbTraits>::processPixel(int x, int y)
{
  *m_dstAddress = rgba_blender_normal(*m_srcAddress,
                                      *m_tmpAddress,
                                      m_opacity);
  ++m_tmpAddress;
}

template<>
void GradientInkProcessing<GrayscaleTraits>::prepareForStrokes(ToolLoop* loop, Strokes& strokes)
{
  color_t c0 = loop->getPrimaryColor();
  color_t c1 = loop->getSecondaryColor();
  int v0 = int(doc::graya_getv(c0));
  int a0 = int(doc::graya_geta(c0));
  int v1 = int(doc::graya_getv(c1));
  int a1 = int(doc::graya_geta(c1));
  c0 = doc::rgba(v0, v0, v0, a0);
  c1 = doc::rgba(v1, v1, v1, a1);

  renderRgbaGradient(loop, strokes, c0, c1);
}

template<>
void GradientInkProcessing<GrayscaleTraits>::processPixel(int x, int y)
{
  doc::color_t c = *m_tmpAddress;
  int a = doc::rgba_geta(c);
  int v = doc::rgba_getr(c);

  *m_dstAddress = graya_blender_normal(*m_srcAddress,
                                       doc::graya(v, a),
                                       m_opacity);
  ++m_tmpAddress;
}

template<>
void GradientInkProcessing<IndexedTraits>::prepareForStrokes(ToolLoop* loop, Strokes& strokes)
{
  color_t c0 = m_palette->getEntry(loop->getPrimaryColor());
  color_t c1 = m_palette->getEntry(loop->getSecondaryColor());

  renderRgbaGradient(loop, strokes, c0, c1);
}

template<>
void GradientInkProcessing<IndexedTraits>::processPixel(int x, int y)
{
  doc::color_t c = *m_tmpAddress;
  doc::color_t c0 = *m_srcAddress;
  if (int(c0) == m_maskIndex)
    c0 = m_palette->getEntry(c0) & rgba_rgb_mask;  // Alpha = 0
  else
    c0 = m_palette->getEntry(c0);
  c = rgba_blender_normal(c0, c, m_opacity);

  *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                     rgba_getg(c),
                                     rgba_getb(c),
                                     rgba_geta(c));

  ++m_tmpAddress;
}


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
    m_brushMask = m_brush->maskBitmap();
    m_opacity = loop->getOpacity();
    m_width = m_brush->bounds().w;
    m_height = m_brush->bounds().h;
    m_u = (m_brush->patternOrigin().x - loop->getCelOrigin().x) % m_width;
    m_v = (m_brush->patternOrigin().y - loop->getCelOrigin().y) % m_height;
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    if ((m_brush->pattern() == BrushPattern::ALIGNED_TO_DST && firstPoint) ||
        (m_brush->pattern() == BrushPattern::PAINT_BRUSH)) {
      m_u = (m_brush->patternOrigin().x - loop->getCelOrigin().x) % m_width;
      m_v = (m_brush->patternOrigin().y - loop->getCelOrigin().y) % m_height;
    }
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
  const Image* m_brushMask;
  int m_opacity;
  int m_u, m_v, m_width, m_height;
};

template<>
void BrushInkProcessing<RgbTraits>::processPixel(int x, int y) {
  alignPixelPoint(x, y);
  if (m_brushMask && !get_pixel_fast<BitmapTraits>(m_brushMask, x, y))
    return;

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
      // TODO review this line
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
  if (m_brushMask && !get_pixel_fast<BitmapTraits>(m_brushMask, x, y))
    return;

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
  if (m_brushMask && !get_pixel_fast<BitmapTraits>(m_brushMask, x, y))
    return;

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
      c = m_palette->findBestfit(graya_getv(c),
                                 graya_getv(c),
                                 graya_getv(c),
                                 graya_geta(c), 0);
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

typedef std::unique_ptr<BaseInkProcessing> InkProcessingPtr;

template<template<typename> class T>
BaseInkProcessing* get_ink_proc(ToolLoop* loop)
{
  switch (loop->sprite()->pixelFormat()) {
    case IMAGE_RGB:       return new T<RgbTraits>(loop);
    case IMAGE_GRAYSCALE: return new T<GrayscaleTraits>(loop);
    case IMAGE_INDEXED:   return new T<IndexedTraits>(loop);
  }
  ASSERT(false);
  return nullptr;
}

} // anonymous namespace
} // namespace tools
} // namespace app
