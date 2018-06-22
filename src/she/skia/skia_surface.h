// SHE library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SURFACE_INCLUDED
#define SHE_SKIA_SKIA_SURFACE_INCLUDED
#pragma once

#include "base/exception.h"
#include "gfx/clip.h"
#include "she/common/generic_surface.h"
#include "she/common/sprite_sheet_font.h"

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColorFilter.h"
#include "SkColorPriv.h"
#include "SkImageInfo.h"
#include "SkRegion.h"
#include "SkSurface.h"

#ifndef SK_BGRA_B32_SHIFT
  #ifdef SK_CPU_BENDIAN
    #define SK_BGRA_B32_SHIFT   24
    #define SK_BGRA_G32_SHIFT   16
    #define SK_BGRA_R32_SHIFT   8
    #define SK_BGRA_A32_SHIFT   0
  #else
    #define SK_BGRA_B32_SHIFT   0
    #define SK_BGRA_G32_SHIFT   8
    #define SK_BGRA_R32_SHIFT   16
    #define SK_BGRA_A32_SHIFT   24
  #endif
#endif

#ifndef SK_A4444_SHIFT
  #define SK_R4444_SHIFT    12
  #define SK_G4444_SHIFT    8
  #define SK_B4444_SHIFT    4
  #define SK_A4444_SHIFT    0
#endif

namespace she {

inline SkColor to_skia(gfx::Color c) {
  return SkColorSetARGBInline(gfx::geta(c), gfx::getr(c), gfx::getg(c), gfx::getb(c));
}

inline SkIRect to_skia(const gfx::Rect& rc) {
  return SkIRect::MakeXYWH(rc.x, rc.y, rc.w, rc.h);
}

class SkiaSurface : public Surface {
public:
  SkiaSurface() : m_surface(nullptr)
                , m_canvas(nullptr)
                , m_lock(0) {
  }

  SkiaSurface(const sk_sp<SkSurface>& surface)
    : m_surface(surface)
    , m_canvas(nullptr)
    , m_lock(0)
  {
    ASSERT(m_surface);
    if (m_surface)
      m_canvas = m_surface->getCanvas();
  }

  ~SkiaSurface() {
    ASSERT(m_lock == 0);
    if (!m_surface)
      delete m_canvas;
  }

  void create(int width, int height) {
    ASSERT(!m_surface);

    if (!m_bitmap.tryAllocPixels(
          SkImageInfo::MakeN32(width, height, kOpaque_SkAlphaType,
                               colorSpace())))
      throw base::Exception("Cannot create Skia surface");

    m_bitmap.eraseColor(SK_ColorTRANSPARENT);
    rebuild();
  }

  void createRgba(int width, int height) {
    ASSERT(!m_surface);

    if (!m_bitmap.tryAllocPixels(
          SkImageInfo::MakeN32Premul(width, height, colorSpace())))
      throw base::Exception("Cannot create Skia surface");

    m_bitmap.eraseColor(SK_ColorTRANSPARENT);
    rebuild();
  }

  void flush() {
    if (m_canvas)
      m_canvas->flush();
  }

  // Surface impl

  void dispose() override {
    delete this;
  }

  int width() const override {
    if (m_surface)
      return m_surface->width();
    else
      return m_bitmap.width();
  }

  int height() const override {
    if (m_surface)
      return m_surface->height();
    else
      return m_bitmap.height();
  }

  bool isDirectToScreen() const override {
    return false;
  }

  int getSaveCount() const override {
    return m_canvas->getSaveCount();
  }

  gfx::Rect getClipBounds() const override {
    SkIRect rc;
    if (m_canvas->getDeviceClipBounds(&rc))
      return gfx::Rect(rc.x(), rc.y(), rc.width(), rc.height());
    else
      return gfx::Rect();
  }

  void saveClip() override {
    m_canvas->save();
  }

  void restoreClip() override {
    m_canvas->restore();
  }

  bool clipRect(const gfx::Rect& rc) override {
    m_canvas->clipRect(SkRect::Make(to_skia(rc)));
    return !m_canvas->isClipEmpty();
  }

  void setDrawMode(DrawMode mode, int param,
                   const gfx::Color a,
                   const gfx::Color b) override {
    switch (mode) {
      case DrawMode::Solid:
        m_paint.setBlendMode(SkBlendMode::kSrcOver);
        m_paint.setShader(nullptr);
        break;
      case DrawMode::Xor:
        m_paint.setBlendMode(SkBlendMode::kXor);
        m_paint.setShader(nullptr);
        break;
      case DrawMode::Checked: {
        m_paint.setBlendMode(SkBlendMode::kSrcOver);
        {
          SkBitmap bitmap;
          if (!bitmap.tryAllocPixels(
                SkImageInfo::MakeN32(8, 8, kOpaque_SkAlphaType, colorSpace()))) {
            throw base::Exception("Cannot create temporary Skia surface");
          }

          {
            SkPMColor A = SkPreMultiplyARGB(gfx::geta(a), gfx::getr(a), gfx::getg(a), gfx::getb(a));
            SkPMColor B = SkPreMultiplyARGB(gfx::geta(b), gfx::getr(b), gfx::getg(b), gfx::getb(b));
            int offset = 7 - (param & 7);
            for (int y=0; y<8; y++)
              for (int x=0; x<8; x++)
                *bitmap.getAddr32(x, y) = (((x+y+offset)&7) < 4 ? B: A);
          }

          sk_sp<SkShader> shader(
            SkShader::MakeBitmapShader(
              bitmap,
              SkShader::kRepeat_TileMode,
              SkShader::kRepeat_TileMode));
          m_paint.setShader(shader);
        }
        break;
      }
    }
  }

  void lock() override {
    ASSERT(m_lock >= 0);
    if (m_lock++ == 0) {
      // m_bitmap is always locked
    }
  }

  void unlock() override {
    ASSERT(m_lock > 0);
    if (--m_lock == 0) {
      // m_bitmap is always locked
    }
  }

  void applyScale(int scaleFactor) override {
    ASSERT(!m_surface);

    SkBitmap result;
    if (!result.tryAllocPixels(
          m_bitmap.info().makeWH(
            width()*scaleFactor,
            height()*scaleFactor)))
      throw base::Exception("Cannot create temporary Skia surface to change scale");

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    SkCanvas canvas(result);
    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, width(), height()));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, result.width(), result.height()));
    canvas.drawBitmapRect(m_bitmap, srcRect, dstRect, &paint,
                          SkCanvas::kStrict_SrcRectConstraint);

    swapBitmap(result);
  }

  void* nativeHandle() override {
    return (void*)this;
  }

  void clear() override {
    m_canvas->clear(0);
  }

  uint8_t* getData(int x, int y) const override {
    if (m_bitmap.isNull())
      return nullptr;
    else
      return (uint8_t*)m_bitmap.getAddr32(x, y);
  }

  void getFormat(SurfaceFormatData* formatData) const override {
    formatData->format = kRgbaSurfaceFormat;
    formatData->bitsPerPixel = 8*m_bitmap.bytesPerPixel();

    switch (m_bitmap.colorType()) {
      case kRGB_565_SkColorType:
        formatData->redShift   = SK_R16_SHIFT;
        formatData->greenShift = SK_G16_SHIFT;
        formatData->blueShift  = SK_B16_SHIFT;
        formatData->alphaShift = 0;
        formatData->redMask    = SK_R16_MASK;
        formatData->greenMask  = SK_G16_MASK;
        formatData->blueMask   = SK_B16_MASK;
        formatData->alphaMask  = 0;
        break;
      case kARGB_4444_SkColorType:
        formatData->redShift   = SK_R4444_SHIFT;
        formatData->greenShift = SK_G4444_SHIFT;
        formatData->blueShift  = SK_B4444_SHIFT;
        formatData->alphaShift = SK_A4444_SHIFT;
        formatData->redMask    = (15 << SK_R4444_SHIFT);
        formatData->greenMask  = (15 << SK_G4444_SHIFT);
        formatData->blueMask   = (15 << SK_B4444_SHIFT);
        formatData->alphaMask  = (15 << SK_A4444_SHIFT);
        break;
      case kRGBA_8888_SkColorType:
        formatData->redShift   = SK_RGBA_R32_SHIFT;
        formatData->greenShift = SK_RGBA_G32_SHIFT;
        formatData->blueShift  = SK_RGBA_B32_SHIFT;
        formatData->alphaShift = SK_RGBA_A32_SHIFT;
        formatData->redMask    = (255 << SK_RGBA_R32_SHIFT);
        formatData->greenMask  = (255 << SK_RGBA_G32_SHIFT);
        formatData->blueMask   = (255 << SK_RGBA_B32_SHIFT);
        formatData->alphaMask  = (255 << SK_RGBA_A32_SHIFT);
        break;
      case kBGRA_8888_SkColorType:
        formatData->redShift   = SK_BGRA_R32_SHIFT;
        formatData->greenShift = SK_BGRA_G32_SHIFT;
        formatData->blueShift  = SK_BGRA_B32_SHIFT;
        formatData->alphaShift = SK_BGRA_A32_SHIFT;
        formatData->redMask    = (255 << SK_BGRA_R32_SHIFT);
        formatData->greenMask  = (255 << SK_BGRA_G32_SHIFT);
        formatData->blueMask   = (255 << SK_BGRA_B32_SHIFT);
        formatData->alphaMask  = (255 << SK_BGRA_A32_SHIFT);
        break;
      default:
        formatData->redShift   = 0;
        formatData->greenShift = 0;
        formatData->blueShift  = 0;
        formatData->alphaShift = 0;
        formatData->redMask    = 0;
        formatData->greenMask  = 0;
        formatData->blueMask   = 0;
        formatData->alphaMask  = 0;
        break;
    }
  }

  gfx::Color getPixel(int x, int y) const override {
    SkColor c = 0;

    if (m_surface) {
      SkImageInfo dstInfo = SkImageInfo::MakeN32Premul(1, 1, colorSpace());
      uint32_t dstPixels;
      if (m_canvas->readPixels(dstInfo, &dstPixels, 4, x, y))
        c = dstPixels;
    }
    else
      c = m_bitmap.getColor(x, y);

    return gfx::rgba(
      SkColorGetR(c),
      SkColorGetG(c),
      SkColorGetB(c),
      SkColorGetA(c));
  }

  void putPixel(gfx::Color color, int x, int y) override {
    m_paint.setColor(to_skia(color));
    m_canvas->drawPoint(SkIntToScalar(x), SkIntToScalar(y), m_paint);
  }

  void drawHLine(gfx::Color color, int x, int y, int w) override {
    m_paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(x), SkIntToScalar(y),
      SkIntToScalar(x+w), SkIntToScalar(y), m_paint);
  }

  void drawVLine(gfx::Color color, int x, int y, int h) override {
    m_paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(x), SkIntToScalar(y),
      SkIntToScalar(x), SkIntToScalar(y+h), m_paint);
  }

  void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) override {
    m_paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(a.x), SkIntToScalar(a.y),
      SkIntToScalar(b.x), SkIntToScalar(b.y), m_paint);
  }

  void drawRect(gfx::Color color, const gfx::Rect& rc) override {
    m_paint.setColor(to_skia(color));
    m_paint.setStyle(SkPaint::kStroke_Style);
    m_canvas->drawIRect(to_skia(gfx::Rect(rc.x, rc.y, rc.w-1, rc.h-1)), m_paint);
  }

  void fillRect(gfx::Color color, const gfx::Rect& rc) override {
    m_paint.setColor(to_skia(color));
    m_paint.setStyle(SkPaint::kFill_Style);
    m_canvas->drawIRect(to_skia(rc), m_paint);
  }

  void blitTo(Surface* _dst, int srcx, int srcy, int dstx, int dsty, int width, int height) const override {
    ASSERT(!m_bitmap.empty());

    auto dst = static_cast<SkiaSurface*>(_dst);

    SkIRect srcRect = SkIRect::MakeXYWH(srcx, srcy, width, height);
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(dstx, dsty, width, height));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    dst->m_canvas->drawBitmapRect(m_bitmap, srcRect, dstRect, &paint,
                                  SkCanvas::kStrict_SrcRectConstraint);
  }

  void scrollTo(const gfx::Rect& rc, int dx, int dy) override {
    int w = width();
    int h = height();
    gfx::Clip clip(rc.x+dx, rc.y+dy, rc);
    if (!clip.clip(w, h, w, h))
      return;

    if (m_surface) {
      SurfaceLock lock(this);
      blitTo(this, clip.src.x, clip.src.y, clip.dst.x, clip.dst.y, clip.size.w, clip.size.h);
      return;
    }

    int bytesPerPixel = m_bitmap.bytesPerPixel();
    int rowBytes = (int)m_bitmap.rowBytes();
    int rowDelta;

    if (dy > 0) {
      clip.src.y += clip.size.h-1;
      clip.dst.y += clip.size.h-1;
      rowDelta = -rowBytes;
    }
    else
      rowDelta = rowBytes;

    char* dst = (char*)m_bitmap.getPixels();
    const char* src = dst;
    dst += rowBytes*clip.dst.y + bytesPerPixel*clip.dst.x;
    src += rowBytes*clip.src.y + bytesPerPixel*clip.src.x;
    w = bytesPerPixel*clip.size.w;
    h = clip.size.h;

    while (--h >= 0) {
      memmove(dst, src, w);
      dst += rowDelta;
      src += rowDelta;
    }

    m_bitmap.notifyPixelsChanged();
  }

  void drawSurface(const Surface* src, int dstx, int dsty) override {
    gfx::Clip clip(dstx, dsty, 0, 0,
      ((SkiaSurface*)src)->width(),
      ((SkiaSurface*)src)->height());

    if (!clip.clip(width(), height(), clip.size.w, clip.size.h))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawRgbaSurface(const Surface* src, int dstx, int dsty) override {
    gfx::Clip clip(dstx, dsty, 0, 0,
      ((SkiaSurface*)src)->width(),
      ((SkiaSurface*)src)->height());

    if (!clip.clip(width(), height(), clip.size.w, clip.size.h))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawRgbaSurface(const Surface* src, int srcx, int srcy, int dstx, int dsty, int w, int h) override {
    gfx::Clip clip(dstx, dsty, srcx, srcy, w, h);
    if (!clip.clip(width(), height(), src->width(), src->height()))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawRgbaSurface(const Surface* src, const gfx::Rect& srcRect, const gfx::Rect& dstRect) override {
    SkRect srcRect2 = SkRect::Make(SkIRect::MakeXYWH(srcRect.x, srcRect.y, srcRect.w, srcRect.h));
    SkRect dstRect2 = SkRect::Make(SkIRect::MakeXYWH(dstRect.x, dstRect.y, dstRect.w, dstRect.h));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);
    paint.setFilterQuality(srcRect.w < dstRect.w ||
                           srcRect.h < dstRect.h ? kNone_SkFilterQuality:
                                                   kHigh_SkFilterQuality);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect2, dstRect2, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawColoredRgbaSurface(const Surface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clipbase) override {
    gfx::Clip clip(clipbase);
    if (!clip.clip(width(), height(), src->width(), src->height()))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);

    if (gfx::geta(bg) > 0) {
      SkPaint paint;
      paint.setColor(to_skia(bg));
      paint.setStyle(SkPaint::kFill_Style);
      m_canvas->drawRect(dstRect, paint);
    }

    sk_sp<SkColorFilter> colorFilter(
      SkColorFilter::MakeModeFilter(to_skia(fg), SkBlendMode::kSrcIn));
    paint.setColorFilter(colorFilter);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap,
      srcRect, dstRect, &paint);
  }

  SkBitmap& bitmap() {
    return m_bitmap;
  }

  void swapBitmap(SkBitmap& other) {
    ASSERT(!m_surface);

    m_bitmap.swap(other);
    rebuild();
  }

  static Surface* loadSurface(const char* filename);

private:
  void rebuild() {
    ASSERT(!m_surface);

    delete m_canvas;
    m_canvas = new SkCanvas(m_bitmap);
  }

  static sk_sp<SkColorSpace> colorSpace();

  SkBitmap m_bitmap;
  sk_sp<SkSurface> m_surface;
  SkCanvas* m_canvas;
  SkPaint m_paint;
  int m_lock;
  static sk_sp<SkColorSpace> m_colorSpace;

};

} // namespace she

#endif
