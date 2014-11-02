/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "app/modules/palettes.h"
#include "app/settings/document_settings.h"
#include "app/tools/shade_table.h"
#include "app/tools/shading_options.h"
#include "filters/neighboring_pixels.h"
#include "raster/palette.h"
#include "raster/raster.h"
#include "raster/rgbmap.h"
#include "raster/sprite.h"

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
// Opaque Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class OpaqueInkProcessing : public SimpleInkProcessing<OpaqueInkProcessing<ImageTraits>, ImageTraits> {
public:
  OpaqueInkProcessing(ToolLoop* loop) {
    m_color = loop->getPrimaryColor();
  }

  void processPixel(int x, int y) {
    *SimpleInkProcessing<OpaqueInkProcessing<ImageTraits>, ImageTraits>::m_dstAddress = m_color;
  }

private:
  color_t m_color;
};

//////////////////////////////////////////////////////////////////////
// SetAlpha Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class SetAlphaInkProcessing : public SimpleInkProcessing<SetAlphaInkProcessing<ImageTraits>, ImageTraits> {
public:
  SetAlphaInkProcessing(ToolLoop* loop) {
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
void SetAlphaInkProcessing<RgbTraits>::processPixel(int x, int y) {
  *m_dstAddress = rgba(rgba_getr(m_color),
                       rgba_getg(m_color),
                       rgba_getb(m_color),
                       m_opacity);
}

template<>
void SetAlphaInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya(graya_getv(m_color), m_opacity);
}

template<>
void SetAlphaInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  *m_dstAddress = m_color;
}

//////////////////////////////////////////////////////////////////////
// LockAlpha Ink
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class LockAlphaInkProcessing : public DoubleInkProcessing<LockAlphaInkProcessing<ImageTraits>, ImageTraits> {
public:
  LockAlphaInkProcessing(ToolLoop* loop) {
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
void LockAlphaInkProcessing<RgbTraits>::processPixel(int x, int y) {
  color_t result = rgba_blend_normal(*m_srcAddress, m_color, m_opacity);
  *m_dstAddress = rgba(
    rgba_getr(result),
    rgba_getg(result),
    rgba_getb(result),
    rgba_geta(*m_srcAddress));
}

template<>
void LockAlphaInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t result = graya_blend_normal(*m_srcAddress, m_color, m_opacity);
  *m_dstAddress = graya(
    graya_getv(result),
    graya_geta(*m_srcAddress));
}

template<>
void LockAlphaInkProcessing<IndexedTraits>::processPixel(int x, int y) {
  *m_dstAddress = m_color;
}

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
  *m_dstAddress = rgba_blend_normal(*m_srcAddress, m_color, m_opacity);
}

template<>
void TransparentInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blend_normal(*m_srcAddress, m_color, m_opacity);
}

template<>
class TransparentInkProcessing<IndexedTraits> : public DoubleInkProcessing<TransparentInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  TransparentInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_rgbmap(loop->getRgbMap()),
    m_opacity(loop->getOpacity()),
    m_color(m_palette->getEntry(loop->getPrimaryColor())) {
  }

  void processPixel(int x, int y) {
    color_t c = rgba_blend_normal(m_palette->getEntry(*m_srcAddress), m_color, m_opacity);
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  int m_opacity;
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
    m_tiledMode(loop->getDocumentSettings()->getTiledMode()),
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
    m_tiledMode(loop->getDocumentSettings()->getTiledMode()),
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
    m_tiledMode(loop->getDocumentSettings()->getTiledMode()),
    m_srcImage(loop->getSrcImage()),
    m_area(get_current_palette()) {
  }

  void processPixel(int x, int y) {
    m_area.reset();
    get_neighboring_pixels<IndexedTraits>(m_srcImage, x, y, 3, 3, 1, 1, m_tiledMode, m_area);

    if (m_area.count > 0 && m_area.a/9 >= 128) {
      m_area.r /= m_area.count;
      m_area.g /= m_area.count;
      m_area.b /= m_area.count;

      uint32_t color32 = m_palette->getEntry(*m_srcAddress);
      m_area.r = rgba_getr(color32) + (m_area.r-rgba_getr(color32)) * m_opacity / 255;
      m_area.g = rgba_getg(color32) + (m_area.g-rgba_getg(color32)) * m_opacity / 255;
      m_area.b = rgba_getb(color32) + (m_area.b-rgba_getb(color32)) * m_opacity / 255;

      *m_dstAddress = m_rgbmap->mapColor(m_area.r, m_area.g, m_area.b);
    }
    else {
      *m_dstAddress = *m_srcAddress;
    }
  }

private:
  struct GetPixelsDelegate {
    const Palette* pal;
    int count, r, g, b, a;

    GetPixelsDelegate(const Palette* pal) : pal(pal) { }

    void reset() { count = r = g = b = a = 0; }

    void operator()(IndexedTraits::pixel_t color)
    {
      a += (color == 0 ? 0: 255);

      uint32_t color32 = pal->getEntry(color);
      r += rgba_getr(color32);
      g += rgba_getg(color32);
      b += rgba_getb(color32);
      count++;
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
    *m_dstAddress = rgba_blend_merge(src, m_color2, m_opacity);
  }
}

template<>
void ReplaceInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  color_t src = (*m_srcAddress);

  if ((graya_geta(src) == 0 && graya_geta(m_color1) == 0) ||
      (graya_geta(src) > 0 && graya_geta(m_color1) > 0 &&
       ((src & graya_v_mask) == (m_color1 & graya_v_mask)))) {
    *m_dstAddress = graya_blend_merge(src, m_color2, m_opacity);
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
        color_t c = rgba_blend_normal(
          m_palette->getEntry(*m_srcAddress), m_color2, m_opacity);

        *m_dstAddress = m_rgbmap->mapColor(
          rgba_getr(c), rgba_getg(c), rgba_getb(c));
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
    m_tiledMode(loop->getDocumentSettings()->getTiledMode()),
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

    if (m_tiledMode & TILED_X_AXIS) {
      if (u < 0)
        u = m_srcImageWidth - (-(u+1) % m_srcImageWidth) - 1;
      else if (u >= m_srcImageWidth)
        u %= m_srcImageWidth;
    }
    else {
      u = MID(0, u, m_srcImageWidth-1);
    }

    if (m_tiledMode & TILED_Y_AXIS) {
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
  Image* m_srcImage;
  int m_srcImageWidth;
  int m_srcImageHeight;
  color_t m_color;
};

template<>
void JumbleInkProcessing<RgbTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);
  *m_dstAddress = rgba_blend_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
void JumbleInkProcessing<GrayscaleTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);
  *m_dstAddress = graya_blend_merge(*m_srcAddress, m_color, m_opacity);
}

template<>
void JumbleInkProcessing<IndexedTraits>::processPixel(int x, int y)
{
  pickColorFromArea(x, y);

  color_t tc = (m_color != 0 ? m_palette->getEntry(m_color): 0);
  color_t c = rgba_blend_merge(*m_srcAddress != 0 ?
                               m_palette->getEntry(*m_srcAddress): 0,
                               tc, m_opacity);

  if (rgba_geta(c) >= 128)
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c));
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
class ShadingInkProcessing<IndexedTraits> : public DoubleInkProcessing<ShadingInkProcessing<IndexedTraits>, IndexedTraits> {
public:
  ShadingInkProcessing(ToolLoop* loop) :
    m_palette(get_current_palette()),
    m_shadeTable(loop->getShadingOptions()->getShadeTable()),
    m_left(loop->getMouseButton() == ToolLoop::Left) {
  }

  void processPixel(int x, int y) {
    if (m_left)
      *m_dstAddress = m_shadeTable->left(*m_srcAddress);
    else
      *m_dstAddress = m_shadeTable->right(*m_srcAddress);
  }

private:
  const Palette* m_palette;
  tools::ShadeTable8* m_shadeTable;
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
  *m_dstAddress = rgba_blend_blackandwhite(*m_srcAddress, m_color, 255);
}

template<>
void XorInkProcessing<GrayscaleTraits>::processPixel(int x, int y) {
  *m_dstAddress = graya_blend_blackandwhite(*m_srcAddress, m_color, 255);
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
    color_t c = rgba_blend_blackandwhite(m_palette->getEntry(*m_srcAddress), m_color, 255);
    *m_dstAddress = m_rgbmap->mapColor(rgba_getr(c),
                                       rgba_getg(c),
                                       rgba_getb(c));
  }

private:
  const Palette* m_palette;
  const RgbMap* m_rgbmap;
  color_t m_color;
};

//////////////////////////////////////////////////////////////////////

enum {
  INK_OPAQUE,
  INK_SETALPHA,
  INK_LOCKALPHA,
  INK_TRANSPARENT,
  INK_BLUR,
  INK_REPLACE,
  INK_JUMBLE,
  INK_SHADING,
  INK_XOR,
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

  DEFINE_INK(OpaqueInkProcessing),
  DEFINE_INK(SetAlphaInkProcessing),
  DEFINE_INK(LockAlphaInkProcessing),
  DEFINE_INK(TransparentInkProcessing),
  DEFINE_INK(BlurInkProcessing),
  DEFINE_INK(ReplaceInkProcessing),
  DEFINE_INK(JumbleInkProcessing),
  DEFINE_INK(ShadingInkProcessing),
  DEFINE_INK(XorInkProcessing)
};

} // anonymous namespace
} // namespace tools
} // namespace app
