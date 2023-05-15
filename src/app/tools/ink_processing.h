// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/color_utils.h"
#include "app/util/wrap_point.h"
#include "app/util/wrap_value.h"
#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
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

class BaseInkProcessing {
public:
  virtual ~BaseInkProcessing() { }
  virtual void processScanline(int x1, int y, int x2, ToolLoop* loop) = 0;
  virtual void prepareForStrokes(ToolLoop* loop, Strokes& strokes) { }
  virtual void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) { }
  virtual void prepareVForPointShape(ToolLoop* loop, int y) { }
  virtual void prepareUForPointShapeWholeScanline(ToolLoop* loop, int x1) { }
  virtual void prepareUForPointShapeSlicedScanline(ToolLoop* loop, bool leftSlice, int x1) { }
};

class NoopInkProcessing : public BaseInkProcessing {
public:
  void processScanline(int x1, int y, int x2, ToolLoop* loop) override { }
};

typedef std::unique_ptr<BaseInkProcessing> InkProcessingPtr;

namespace {

//////////////////////////////////////////////////////////////////////
// Ink Processing
//////////////////////////////////////////////////////////////////////

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
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = loop->getPrimaryColor();

    if (loop->getLayer()->isBackground()) {
      switch (loop->sprite()->pixelFormat()) {
        case IMAGE_RGB: m_color |= rgba_a_mask; break;
        case IMAGE_GRAYSCALE: m_color |= graya_a_mask; break;
      }
    }
  }

  void processPixel(int x, int y) {
    *this->m_dstAddress = m_color;
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
    : m_opacity(loop->getOpacity()) {
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = loop->getPrimaryColor();
  }

  void processPixel(int x, int y) {
    // Do nothing
  }

private:
  color_t m_color;
  const int m_opacity;
};

template<>
void LockAlphaInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t result = rgba_blender_normal(*m_srcAddress, m_color, m_opacity);
  *m_dstAddress = doc::rgba(
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
    : m_palette(loop->getPalette())
    , m_rgbmap(loop->getRgbMap())
    , m_opacity(loop->getOpacity())
    , m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()) {
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = m_palette->getEntry(loop->getPrimaryColor());
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
  color_t m_color;
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
    m_opacity = loop->getOpacity();
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = loop->getPrimaryColor();
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
    m_palette(loop->getPalette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()),
    m_colorIndex(loop->getFgColor()) {
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = m_palette->getEntry(loop->getPrimaryColor());
  }

  void processPixel(int x, int y) {
    if (m_colorIndex == m_maskIndex)
      return;

    color_t c = *m_srcAddress;
    if (int(c) == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    c = rgba_blender_normal(c, m_color, m_opacity);
    *m_dstAddress = m_rgbmap->mapColor(c);
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const int m_opacity;
  color_t m_color;
  const int m_maskIndex;
  int m_colorIndex;
};

//////////////////////////////////////////////////////////////////////
// Merge Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class MergeInkProcessing : public DoubleInkProcessing<MergeInkProcessing<ImageTraits>, ImageTraits> {
public:
  MergeInkProcessing(ToolLoop* loop) {
    m_opacity = loop->getOpacity();
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = loop->getPrimaryColor();
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
    m_palette(loop->getPalette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor()) {
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    m_color = (int(loop->getPrimaryColor()) == m_maskIndex ?
               (m_palette->getEntry(loop->getPrimaryColor()) & rgba_rgb_mask):
               (m_palette->getEntry(loop->getPrimaryColor())));
  }

  void processPixel(int x, int y) {
    color_t c = *m_srcAddress;
    if (int(c) == m_maskIndex)
      c = m_palette->getEntry(c) & rgba_rgb_mask;  // Alpha = 0
    else
      c = m_palette->getEntry(c);

    c = rgba_blender_merge(c, m_color, m_opacity);
    *m_dstAddress = m_rgbmap->mapColor(c);
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  const int m_opacity;
  const int m_maskIndex;
  color_t m_color;
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
        rgba_blender_merge(*m_srcAddress,
                           doc::rgba(m_area.r, m_area.g, m_area.b, m_area.a),
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

    void operator()(RgbTraits::pixel_t color) {
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
        graya_blender_merge(*m_srcAddress,
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
    m_palette(loop->getPalette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_tiledMode(loop->getTiledMode()),
    m_srcImage(loop->getSrcImage()),
    m_area(loop->getPalette(),
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

      const color_t c =
        rgba_blender_merge(m_palette->getEntry(*m_srcAddress),
                           doc::rgba(m_area.r, m_area.g, m_area.b, m_area.a),
                           m_opacity);

      *m_dstAddress = m_rgbmap->mapColor(c);
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
    m_palette = loop->getPalette();
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

        *m_dstAddress = m_rgbmap->mapColor(c);
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

template<>
void ReplaceInkProcessing<TilemapTraits>::processPixel(int x, int y) {
  color_t c = *m_srcAddress;
  *m_dstAddress = (c == m_color1 ? m_color2: c);
}

//////////////////////////////////////////////////////////////////////
// Jumble Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class JumbleInkProcessing : public DoubleInkProcessing<JumbleInkProcessing<ImageTraits>, ImageTraits> {
public:
  JumbleInkProcessing(ToolLoop* loop) :
    m_palette(loop->getPalette()),
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
                              m_srcImageHeight),
                    pt, false);

    pt.x = std::clamp(pt.x, 0, m_srcImageWidth-1);
    pt.y = std::clamp(pt.y, 0, m_srcImageHeight-1);

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
    *m_dstAddress = m_rgbmap->mapColor(c);
  else
    *m_dstAddress = 0;
}

//////////////////////////////////////////////////////////////////////
// Shading Ink Helper used by ShadingInkProcessing and
// BrushShadingInkProcessing (it does only the shading of one pixel
// using some auxiliary structures)
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class PixelShadingInkHelper {
public:
  PixelShadingInkHelper(ToolLoop* loop) {
    static_assert(false && sizeof(ImageTraits), "Use specialized cases");
  }
};

template<>
class PixelShadingInkHelper<RgbTraits> {
public:
  using pixel_t = RgbTraits::pixel_t;

  PixelShadingInkHelper(ToolLoop* loop)
    : m_shadePalette(0, 1)
    , m_left(loop->getMouseButton() == ToolLoop::Left)
  {
    const Shade shade = loop->getShade();
    m_shadePalette.resize(shade.size());
    int i = 0;
    for (app::Color color : shade) {
      m_shadePalette.setEntry(
        i++, color_utils::color_for_layer(color, loop->getLayer()));
    }
  }

  int findIndex(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const {
    return m_shadePalette.findExactMatch(r, g, b, a, -1);
  }

  pixel_t operator()(const pixel_t src) const {
    int i = findIndex(rgba_getr(src),
                      rgba_getg(src),
                      rgba_getb(src),
                      rgba_geta(src));
    if (i < 0)
      return src;

    if (m_left) {
      if (i > 0)
        --i;
    }
    else {
      if (i < m_shadePalette.size()-1)
        ++i;
    }
    return m_shadePalette.getEntry(i);
  }

private:
  Palette m_shadePalette;
  bool m_left;
};

template<>
class PixelShadingInkHelper<GrayscaleTraits> {
public:
  using pixel_t = GrayscaleTraits::pixel_t;

  PixelShadingInkHelper(ToolLoop* loop)
    : m_shadePalette(0, 1)
    , m_left(loop->getMouseButton() == ToolLoop::Left)
  {
    Shade shade = loop->getShade();
    m_shadePalette.resize(shade.size());

    // As the colors are going to a palette, we need RGB colors
    // (instead of Grayscale)
    const ColorTarget target(
      (loop->getLayer()->isBackground() ? ColorTarget::BackgroundLayer:
                                          ColorTarget::TransparentLayer),
      IMAGE_RGB, 0);

    int i = 0;
    for (app::Color color : shade) {
      m_shadePalette.setEntry(
        i++, color_utils::color_for_target(color, target));
    }
  }

  int findIndex(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const {
    return m_shadePalette.findExactMatch(r, g, b, a, -1);
  }

  pixel_t operator()(const pixel_t src) const {
    int i = findIndex(graya_getv(src),
                      graya_getv(src),
                      graya_getv(src),
                      graya_geta(src));
    if (i < 0)
      return src;

    if (m_left) {
      if (i > 0)
        --i;
    }
    else {
      if (i < m_shadePalette.size()-1)
        ++i;
    }

    color_t rgba = m_shadePalette.getEntry(i);
    return graya(rgba_getr(rgba), rgba_geta(rgba));
  }

private:
  Palette m_shadePalette;
  bool m_left;
};

template<>
class PixelShadingInkHelper<IndexedTraits> {
public:
  using pixel_t = IndexedTraits::pixel_t;

  PixelShadingInkHelper(ToolLoop* loop) :
    m_palette(loop->getPalette()),
    m_remap(loop->getShadingRemap()),
    m_left(loop->getMouseButton() == ToolLoop::Left) {
  }

  int findIndex(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const {
    return m_palette->findExactMatch(r, g, b, a, -1);
  }

  pixel_t operator()(pixel_t i) const {
    if (m_remap) {
      i = (*m_remap)[i];
    }
    else {
      if (m_left) {
        if (i > 0)
          --i;
      }
      else {
        if (i < m_palette->size()-1)
          ++i;
      }
    }
    return i;
  }

private:
  const Palette* m_palette;
  const Remap* m_remap;
  bool m_left;
};

//////////////////////////////////////////////////////////////////////
// Shading Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class ShadingInkProcessing : public DoubleInkProcessing<ShadingInkProcessing<ImageTraits>, ImageTraits> {
public:
  using Base = DoubleInkProcessing<ShadingInkProcessing<ImageTraits>, ImageTraits>;

  ShadingInkProcessing(ToolLoop* loop) : m_shading(loop) { }
  void processPixel(int x, int y) {
    *Base::m_dstAddress = m_shading(*Base::m_srcAddress);
  }
private:
  PixelShadingInkHelper<ImageTraits> m_shading;
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

    const gfx::Point u = strokes[0].firstPoint().toPoint();
    const gfx::Point v = strokes[0].lastPoint().toPoint();

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

    render::render_rgba_gradient(
      m_tmpImage.get(), imgPos, u, v, c0, c1,
      loop->getDitheringMatrix(),
      loop->getGradientType());
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
    , m_palette(loop->getPalette())
    , m_rgbmap(loop->getRgbMap())
    , m_maskIndex(loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor())
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

  *m_dstAddress = m_rgbmap->mapColor(c);

  ++m_tmpAddress;
}


//////////////////////////////////////////////////////////////////////
// Xor Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class XorInkProcessing : public DoubleInkProcessing<XorInkProcessing<ImageTraits>, ImageTraits> {
public:
  XorInkProcessing(ToolLoop* loop) { }
  void processPixel(int x, int y) { }
};

template<>
void XorInkProcessing<RgbTraits>::processPixel(int x, int y) {
  *m_dstAddress = rgba_blender_neg_bw(*m_srcAddress, 0, 255);
}

template<>
void XorInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blender_neg_bw(*m_srcAddress, 0, 255);
}

template<>
class XorInkProcessing<IndexedTraits> : public DoubleInkProcessing<XorInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  XorInkProcessing(ToolLoop* loop) :
    m_palette(loop->getPalette()),
    m_rgbmap(loop->getRgbMap()) {
  }

  void processPixel(int x, int y) {
    color_t c = rgba_blender_neg_bw(m_palette->getEntry(*m_srcAddress), 0, 255);
    *m_dstAddress = m_rgbmap->mapColor(c);
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
};

//////////////////////////////////////////////////////////////////////
// Brush Ink - Base
//////////////////////////////////////////////////////////////////////

// TODO In all cases where we get the brush index and use that index
//      in m_palette->getEntry(index), the color is converted to the
//      sprite palette or to the grayscale palette (for grayscale
//      sprites), we might want to save the original palette in the
//      brush to use that one in these cases (not sure if this does
//      applies when we select a new foreground/background color in
//      the color bar and the brush color changes, or if this should
//      be a new optional flag/parameter to save on each brush)
template<typename ImageTraits>
class BrushInkProcessingBase : public DoubleInkProcessing<BrushInkProcessingBase<ImageTraits>, ImageTraits> {
public:
  BrushInkProcessingBase(ToolLoop* loop) {
    m_fgColor = loop->getPrimaryColor();
    m_bgColor = loop->getSecondaryColor();
    m_palette = loop->getPalette();
    m_brush = loop->getBrush();
    m_brushImage = (m_brush->patternImage() ? m_brush->patternImage():
                                              m_brush->image());
    m_brushMask = m_brush->maskBitmap();
    m_patternAlign = m_brush->pattern();
    m_opacity = loop->getOpacity();
    m_width = m_brush->bounds().w;
    m_height = m_brush->bounds().h;
    m_u = (m_brush->patternOrigin().x - loop->getCelOrigin().x) % m_width;
    m_v = (m_brush->patternOrigin().y - loop->getCelOrigin().y) % m_height;

    if (loop->sprite()->colorMode() == ColorMode::INDEXED) {
      if (loop->getLayer()->isTransparent())
        m_transparentColor = loop->sprite()->transparentColor();
      else
        m_transparentColor = -1;
    }
    else
      m_transparentColor = 0;
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    if ((m_patternAlign == BrushPattern::ALIGNED_TO_DST && firstPoint) ||
        (m_patternAlign == BrushPattern::PAINT_BRUSH)) {
      m_u = ((m_brush->patternOrigin().x % loop->sprite()->width()) - loop->getCelOrigin().x) % m_width;
      m_v = ((m_brush->patternOrigin().y % loop->sprite()->height()) - loop->getCelOrigin().y) % m_height;
    }
  }

  void prepareVForPointShape(ToolLoop* loop, int y) override {
    if (m_patternAlign == doc::BrushPattern::ALIGNED_TO_SRC) {
      m_v = (m_brush->patternOrigin().y - loop->getCelOrigin().y) % m_height;
      if (m_v < 0) m_v += m_height;
    }
    else {
      int spriteH = loop->sprite()->height();
      if (y/spriteH > 0)
        // 'y' is outside of the center tile.
        m_v = (m_brush->patternOrigin().y + m_height - (y/spriteH) * spriteH) % m_height;
      else
        // 'y' is inside of the center tile.
        m_v = ((m_brush->patternOrigin().y % spriteH) - loop->getCelOrigin().y) % m_height;
    }
  }

  void prepareUForPointShapeWholeScanline(ToolLoop* loop, int x1) override {
    if (m_patternAlign == doc::BrushPattern::ALIGNED_TO_SRC) {
      m_u = (m_brush->patternOrigin().x - loop->getCelOrigin().x) % m_width;
      if (m_u < 0) m_u += m_height;
    }
    else {
      m_u = ((m_brush->patternOrigin().x % loop->sprite()->width()) - loop->getCelOrigin().x ) % m_width;
      if (x1/loop->sprite()->width() > 0)
        m_u = (m_brush->patternOrigin().x + m_width - (x1/loop->sprite()->width()) * loop->sprite()->width()) % m_width;
    }
  }

  void prepareUForPointShapeSlicedScanline(ToolLoop* loop, bool leftSlice, int x1) override {
    if (m_patternAlign == doc::BrushPattern::ALIGNED_TO_SRC) {
      m_u = (m_brush->patternOrigin().x - loop->getCelOrigin().x) % m_width;
      if (m_u < 0) m_u += m_height;
      return;
    }
    else {
      if (leftSlice)
        m_u = ((m_brush->patternOrigin().x % loop->sprite()->width()) - loop->getCelOrigin().x ) % m_width;
      else
        m_u = (m_brush->patternOrigin().x + m_width - (x1/loop->sprite()->width() + 1) * loop->sprite()->width()) % m_width;
    }
  }

  bool preProcessPixel(int x, int y, color_t* result) {
    // Do nothing
    return true;
  }

  // TODO Remove this virtual function in some way. At the moment we
  //      need it because InkProcessing expects that its Derived
  //      template parameter has a processPixel() member function.
  virtual void processPixel(int x, int y) {
    // Do nothing
  }

protected:
  bool alignPixelPoint(int& x0, int& y0) {
    int x = (x0 - m_u) % m_width;
    int y = (y0 - m_v) % m_height;
    if (x < 0) x = m_width - ((-x) % m_width);
    if (y < 0) y = m_height - ((-y) % m_height);

    if (m_brushMask && !get_pixel_fast<BitmapTraits>(m_brushMask, x, y))
      return false;

    if (m_brush->patternImage()) {
      const int w = m_brush->patternImage()->width();
      const int h = m_brush->patternImage()->height();
      x = x0 % w;
      y = y0 % h;
      if (x < 0) x = w - ((-x) % w);
      if (y < 0) y = h - ((-y) % h);
    }

    x0 = x;
    y0 = y;
    return true;
  }

  color_t m_fgColor;
  color_t m_bgColor;
  const Palette* m_palette;
  const Brush* m_brush;
  const Image* m_brushImage;
  const Image* m_brushMask;
  BrushPattern m_patternAlign;
  int m_opacity;
  int m_u, m_v, m_width, m_height;
  // When we have a image brush from an INDEXED sprite, we need to know
  // which is the background color in order to translate to transparent color
  // in a RGBA sprite.
  color_t m_transparentColor;
};

template<>
bool BrushInkProcessingBase<RgbTraits>::preProcessPixel(int x, int y, color_t* result) {
  if (!alignPixelPoint(x, y))
    return false;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);

      // We blend the previous image brush pixel with a pixel from the
      // image preview (*m_dstAddress). Yes, dstImage, in that way we
      // can overlap image brush self printed areas (auto compose
      // colors). Doing this, we avoid eraser action of the pixels
      // with alpha <255 in the image brush.
      c = rgba_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor == c)
        c = 0;
      else
        c = m_palette->getEntry(c);
      c = rgba_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      c = doc::rgba(graya_getv(c), graya_getv(c), graya_getv(c), graya_geta(c));
      c = rgba_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return false;
  }
  *result = c;
  return true;
}

template<>
bool BrushInkProcessingBase<GrayscaleTraits>::preProcessPixel(int x, int y, color_t* result) {
  if (!alignPixelPoint(x, y))
    return false;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      c = graya(rgba_luma(c), rgba_geta(c));
      c = graya_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor == c)
        c = 0;
      else
        c = m_palette->getEntry(c);
      c = graya(rgba_luma(c), rgba_geta(c));
      c = graya_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      c = graya_blender_normal(*m_dstAddress, c, m_opacity);
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return false;
  }
  *result = c;
  return true;
}

template<>
bool BrushInkProcessingBase<IndexedTraits>::preProcessPixel(int x, int y, color_t* result) {
  if (!alignPixelPoint(x, y))
    return false;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      color_t d = m_palette->getEntry(*m_dstAddress);
      c = rgba_blender_normal(d, c, m_opacity);
      c = m_palette->findBestfit(rgba_getr(c),
                                 rgba_getg(c),
                                 rgba_getb(c),
                                 rgba_geta(c), m_transparentColor);
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (c == m_transparentColor)
        return false;

      color_t f = m_palette->getEntry(c);

      // Keep original index in special opaque case
      if (rgba_geta(f) == 255 && m_opacity == 255)
        break;

      color_t b = m_palette->getEntry(*m_dstAddress);
      c = rgba_blender_normal(b, f, m_opacity);
      c = m_palette->findBestfit(rgba_getr(c),
                                 rgba_getg(c),
                                 rgba_getb(c),
                                 rgba_geta(c), m_transparentColor);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      color_t b = m_palette->getEntry(*m_dstAddress);
      b = graya(rgba_luma(b),
                rgba_geta(b));
      c = graya_blender_normal(b, c, m_opacity);
      c = m_palette->findBestfit(graya_getv(c),
                                 graya_getv(c),
                                 graya_getv(c),
                                 graya_geta(c), m_transparentColor);
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return false;
  }
  if (c != m_transparentColor) {
    *result = c;
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////
// Brush Ink - Simple ink type
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushSimpleInkProcessing : public BrushInkProcessingBase<ImageTraits> {
public:
  BrushSimpleInkProcessing(ToolLoop* loop) : BrushInkProcessingBase<ImageTraits>(loop) {
  }

  void processPixel(int x, int y) override {
    // Do nothing
  }
};

template<>
void BrushSimpleInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t c;
  if (preProcessPixel(x, y, &c))
    *m_dstAddress = c;
}

template<>
void BrushSimpleInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  color_t c;
  if (preProcessPixel(x, y, &c))
    *m_dstAddress = c;
}

template<>
void BrushSimpleInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t c;
  if (preProcessPixel(x, y, &c))
    *m_dstAddress = c;
}

//////////////////////////////////////////////////////////////////////
// Brush Ink - Lock Alpha ink type
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushLockAlphaInkProcessing : public BrushInkProcessingBase<ImageTraits> {
public:
  BrushLockAlphaInkProcessing(ToolLoop* loop) : BrushInkProcessingBase<ImageTraits>(loop) {
  }

  void processPixel(int x, int y) override {
    //Do nothing
  }
};

template<>
void BrushLockAlphaInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t c;
  if (preProcessPixel(x, y, &c))
    *m_dstAddress = doc::rgba(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(*m_srcAddress));
}

template<>
void BrushLockAlphaInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  if (*m_srcAddress != m_transparentColor) {
    color_t c;
    if (preProcessPixel(x, y, &c))
      *m_dstAddress = c;
  }
}

template<>
void BrushLockAlphaInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t c;
  if (preProcessPixel(x, y, &c))
    *m_dstAddress = graya(graya_getv(c), graya_geta(*m_srcAddress));
}

//////////////////////////////////////////////////////////////////////
// Brush Ink - Eraser Tool
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushEraserInkProcessing : public BrushInkProcessingBase<ImageTraits> {
public:
  BrushEraserInkProcessing(ToolLoop* loop) : BrushInkProcessingBase<ImageTraits>(loop) {
  }

  void processPixel(int x, int y) override {
    // Do nothing
  }
};

template<>
void BrushEraserInkProcessing<RgbTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      int t;
      c = doc::rgba(rgba_getr(*m_srcAddress),
                    rgba_getg(*m_srcAddress),
                    rgba_getb(*m_srcAddress),
                    MUL_UN8(rgba_geta(*m_dstAddress), 255 - rgba_geta(c), t));
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor == c)
        c = 0;
      else
        c = m_palette->getEntry(c);
      int t;
      c = doc::rgba(rgba_getr(*m_srcAddress),
                    rgba_getg(*m_srcAddress),
                    rgba_getb(*m_srcAddress),
                    MUL_UN8(rgba_geta(*m_dstAddress), 255 - rgba_geta(c), t));
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      int t;
      c = doc::rgba(rgba_getr(*m_srcAddress),
                    rgba_getg(*m_srcAddress),
                    rgba_getb(*m_srcAddress),
                    MUL_UN8(rgba_geta(*m_dstAddress), 255 - graya_geta(c), t));
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_bgColor : *m_srcAddress;
      break;
    }
    default:
      ASSERT(false);
      return;
  }
  *m_dstAddress = c;;
}

template<>
void BrushEraserInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      int t;
      c = graya(graya_getv(*m_srcAddress),
                MUL_UN8(graya_geta(*m_dstAddress), 255 - rgba_geta(c), t));
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor == c)
        c = 0;
      else {
        c = m_palette->getEntry(c);
      }
      int t;
      c = graya(graya_getv(*m_srcAddress),
                MUL_UN8(graya_geta(*m_dstAddress), 255 - rgba_geta(c), t));
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      int t;
      c = graya(graya_getv(*m_srcAddress),
                MUL_UN8(graya_geta(*m_dstAddress), 255 - graya_geta(c), t));
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_bgColor : *m_srcAddress;
      break;
    }
    default:
      ASSERT(false);
      return;
  }
  *m_dstAddress = c;
}

template<>
void BrushEraserInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      c = m_palette->findBestfit(rgba_getr(c),
                                 rgba_getg(c),
                                 rgba_getb(c),
                                 rgba_geta(c), m_transparentColor);
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
                                 graya_geta(c), m_transparentColor);
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
      c = get_pixel_fast<BitmapTraits>(m_brushImage, x, y);
      c = c ? m_fgColor: m_bgColor;
      break;
    }
    default:
      ASSERT(false);
      return;
  }
  if (c != m_transparentColor) {
    *m_dstAddress = m_transparentColor;
  }
}

//////////////////////////////////////////////////////////////////////
// Brush Ink - Shading ink type
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushShadingInkProcessing : public BrushInkProcessingBase<ImageTraits> {
public:
  using pixel_t = typename ImageTraits::pixel_t;

  BrushShadingInkProcessing(ToolLoop* loop)
    : BrushInkProcessingBase<ImageTraits>(loop)
    , m_shading(loop) {
  }

  void processPixel(int x, int y) override {
    // Do nothing
  }

private:
  PixelShadingInkHelper<ImageTraits> m_shading;
};

template <>
void BrushShadingInkProcessing<RgbTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      auto c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      int iBrush = m_shading.findIndex(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
      if (rgba_geta(c) != 0 && iBrush >= 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_INDEXED: {
      auto c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor != c)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_GRAYSCALE: {
      auto c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) != 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    default:
      ASSERT(false);
      return;
  }
};

template <>
void BrushShadingInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      auto c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      int iBrush = m_shading.findIndex(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
      if (rgba_geta(c) != 0 && iBrush >= 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_INDEXED: {
      auto c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor != c)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_GRAYSCALE: {
      auto c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) != 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    default:
      ASSERT(false);
      return;
  }
};

template <>
void BrushShadingInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      auto c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      int iBrush = m_shading.findIndex(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c),
                                       rgba_geta(c));
      if (rgba_geta(c) != 0 && iBrush >= 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_INDEXED: {
      auto c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (m_transparentColor != c)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    case IMAGE_GRAYSCALE: {
      auto c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) != 0)
        *m_dstAddress = m_shading(*m_srcAddress);
      break;
    }
    default:
      ASSERT(false);
      return;
  }
};

//////////////////////////////////////////////////////////////////////
// Brush Ink - Copy Alpha+Color ink type
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class BrushCopyInkProcessing : public BrushInkProcessingBase<ImageTraits> {
public:
  BrushCopyInkProcessing(ToolLoop* loop) : BrushInkProcessingBase<ImageTraits>(loop) {
  }

  void processPixel(int x, int y) override {
    //Do nothing
  }
};

template<>
void BrushCopyInkProcessing<RgbTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      if (rgba_geta(c) == 0)
        return;
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (c == m_transparentColor) {
        *m_dstAddress = *m_srcAddress;
        return;
      }
      c = m_palette->getEntry(c);
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) == 0)
        return;
      c = doc::rgba(graya_getv(c), graya_getv(c), graya_getv(c), graya_geta(c));
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
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

template<>
void BrushCopyInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      if (rgba_geta(c) == 0)
        return;
      c = m_palette->findBestfit(rgba_getr(c),
                                 rgba_getg(c),
                                 rgba_getb(c),
                                 rgba_geta(c), m_transparentColor);
      if (c == 0)
        c = *m_srcAddress;
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (c == m_transparentColor)
        c = *m_srcAddress;
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) == 0)
        return;
      c = m_palette->findBestfit(graya_getv(c),
                                 graya_getv(c),
                                 graya_getv(c),
                                 graya_geta(c), m_transparentColor);
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
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

template<>
void BrushCopyInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  if (!alignPixelPoint(x, y))
    return;

  color_t c;
  switch (m_brushImage->pixelFormat()) {
    case IMAGE_RGB: {
      c = get_pixel_fast<RgbTraits>(m_brushImage, x, y);
      if (rgba_geta(c) == 0)
        return;
      c = graya(rgba_luma(c), rgba_geta(c));
      break;
    }
    case IMAGE_INDEXED: {
      c = get_pixel_fast<IndexedTraits>(m_brushImage, x, y);
      if (c == m_transparentColor) {
        *m_dstAddress = *m_srcAddress;
        return;
      }
      c = m_palette->getEntry(c);
      c = graya(rgba_luma(c), rgba_geta(c));
      break;
    }
    case IMAGE_GRAYSCALE: {
      c = get_pixel_fast<GrayscaleTraits>(m_brushImage, x, y);
      if (graya_geta(c) == 0)
        return;
      break;
    }
    case IMAGE_BITMAP: {
      // TODO In which circuntance is possible this case?
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

template<template<typename> class T>
BaseInkProcessing* get_ink_proc2(ToolLoop* loop)
{
  switch (loop->getDstImage()->pixelFormat()) {
    case IMAGE_RGB:       return new T<RgbTraits>(loop);
    case IMAGE_GRAYSCALE: return new T<GrayscaleTraits>(loop);
    case IMAGE_INDEXED:   return new T<IndexedTraits>(loop);
    case IMAGE_TILEMAP:   return new T<TilemapTraits>(loop);
  }
  ASSERT(false);
  return nullptr;
}

} // anonymous namespace
} // namespace tools
} // namespace app
