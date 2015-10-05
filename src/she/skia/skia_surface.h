// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SURFACE_INCLUDED
#define SHE_SKIA_SKIA_SURFACE_INCLUDED
#pragma once

#include "gfx/clip.h"
#include "she/common/font.h"
#include "she/locked_surface.h"
#include "she/scoped_surface_lock.h"

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColorFilter.h"
#include "SkColorPriv.h"
#include "SkImageInfo.h"
#include "SkRegion.h"
#include "SkSurface.h"

namespace she {

inline SkColor to_skia(gfx::Color c) {
  return SkPremultiplyARGBInline(gfx::geta(c), gfx::getr(c), gfx::getg(c), gfx::getb(c));
}

inline SkIRect to_skia(const gfx::Rect& rc) {
  return SkIRect::MakeXYWH(rc.x, rc.y, rc.w, rc.h);
}

class SkiaSurface : public NonDisposableSurface
                  , public LockedSurface {
public:
  SkiaSurface() : m_surface(nullptr)
                , m_canvas(nullptr) {
  }

  SkiaSurface(SkSurface* surface)
    : m_surface(surface)
    , m_canvas(nullptr)
    , m_clip(0, 0, width(), height())
  {
    ASSERT(m_surface);
    if (m_surface)
      m_canvas = m_surface->getCanvas();
  }

  ~SkiaSurface() {
    if (!m_surface)
      delete m_canvas;
  }

  void create(int width, int height) {
    ASSERT(!m_surface);

    m_bitmap.tryAllocPixels(
      SkImageInfo::MakeN32Premul(width, height));
    m_bitmap.eraseColor(SK_ColorTRANSPARENT);

    rebuild();
  }

  void createRgba(int width, int height) {
    ASSERT(!m_surface);

    m_bitmap.tryAllocPixels(
      SkImageInfo::MakeN32Premul(width, height));
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

  gfx::Rect getClipBounds() override {
    return m_clip;
  }

  void setClipBounds(const gfx::Rect& rc) override {
    m_clip = rc;
    m_canvas->clipRect(SkRect::Make(to_skia(m_clip)), SkRegion::kReplace_Op);
  }

  bool intersectClipRect(const gfx::Rect& rc) override {
    m_clip &= rc;
    m_canvas->clipRect(SkRect::Make(to_skia(m_clip)), SkRegion::kReplace_Op);
    return !m_clip.isEmpty();
  }

  void setDrawMode(DrawMode mode, int param) override {
    // TODO
  }

  LockedSurface* lock() override {
    m_bitmap.lockPixels();
    return this;
  }

  void applyScale(int scaleFactor) override {
    ASSERT(!m_surface);

    SkBitmap result;
    result.tryAllocPixels(
      SkImageInfo::MakeN32Premul(width()*scaleFactor, height()*scaleFactor));

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);

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

  // LockedSurface impl

  int lockedWidth() const override {
    return width();
  }

  int lockedHeight() const override {
    return height();
  }

  void unlock() override {
    m_bitmap.unlockPixels();
  }

  void clear() override {
    m_canvas->clear(0);
  }

  uint8_t* getData(int x, int y) const override {
    return (uint8_t*)m_bitmap.getAddr(x, y);
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
        ASSERT(false);
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
      // m_canvas->flush();

      SkImageInfo dstInfo = SkImageInfo::MakeN32Premul(1, 1);
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
    SkPaint paint;
    paint.setColor(to_skia(color));
    m_canvas->drawPoint(SkIntToScalar(x), SkIntToScalar(y), paint);
  }

  void drawHLine(gfx::Color color, int x, int y, int w) override {
    SkPaint paint;
    paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(x), SkIntToScalar(y),
      SkIntToScalar(x+w), SkIntToScalar(y), paint);
  }

  void drawVLine(gfx::Color color, int x, int y, int h) override {
    SkPaint paint;
    paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(x), SkIntToScalar(y),
      SkIntToScalar(x), SkIntToScalar(y+h), paint);
  }

  void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) override {
    SkPaint paint;
    paint.setColor(to_skia(color));
    m_canvas->drawLine(
      SkIntToScalar(a.x), SkIntToScalar(a.y),
      SkIntToScalar(b.x), SkIntToScalar(b.y), paint);
  }

  void drawRect(gfx::Color color, const gfx::Rect& rc) override {
    SkPaint paint;
    paint.setColor(to_skia(color));
    paint.setStyle(SkPaint::kStroke_Style);
    m_canvas->drawIRect(to_skia(gfx::Rect(rc.x, rc.y, rc.w-1, rc.h-1)), paint);
  }

  void fillRect(gfx::Color color, const gfx::Rect& rc) override {
    SkPaint paint;
    paint.setColor(to_skia(color));
    paint.setStyle(SkPaint::kFill_Style);
    paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
    m_canvas->drawIRect(to_skia(rc), paint);
  }

  void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const override {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    std::vector<uint32_t> pixels(width * height * 4);
    m_canvas->readPixels(info, (void*)&pixels[0], 4*width, srcx, srcy);
    ((SkiaSurface*)dest)->m_canvas->writePixels(info, (void*)&pixels[0], 4*width, dstx, dsty);
  }

  void scrollTo(const gfx::Rect& rc, int dx, int dy) override {
    int w = width();
    int h = height();
    gfx::Clip clip(rc.x+dx, rc.y+dy, rc);
    if (!clip.clip(w, h, w, h))
      return;

    if (m_surface) {
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
  }

  void drawSurface(const LockedSurface* src, int dstx, int dsty) override {
    gfx::Clip clip(dstx, dsty, 0, 0,
      ((SkiaSurface*)src)->width(),
      ((SkiaSurface*)src)->height());

    if (!clip.clip(width(), height(), clip.size.w, clip.size.h))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) override {
    gfx::Clip clip(dstx, dsty, 0, 0,
      ((SkiaSurface*)src)->width(),
      ((SkiaSurface*)src)->height());

    if (!clip.clip(width(), height(), clip.size.w, clip.size.h))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap, srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawColoredRgbaSurface(const LockedSurface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clipbase) override {
    gfx::Clip clip(clipbase);
    if (!clip.clip(lockedWidth(), lockedHeight(), src->lockedWidth(), src->lockedHeight()))
      return;

    SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(clip.src.x, clip.src.y, clip.size.w, clip.size.h));
    SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(clip.dst.x, clip.dst.y, clip.size.w, clip.size.h));

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);

    if (gfx::geta(bg) > 0) {
      SkPaint paint;
      paint.setColor(to_skia(bg));
      paint.setStyle(SkPaint::kFill_Style);
      m_canvas->drawRect(dstRect, paint);
    }

    SkAutoTUnref<SkColorFilter> colorFilter(
      SkColorFilter::CreateModeFilter(to_skia(fg), SkXfermode::kSrcIn_Mode));
    paint.setColorFilter(colorFilter);

    m_canvas->drawBitmapRect(
      ((SkiaSurface*)src)->m_bitmap,
      srcRect, dstRect, &paint,
      SkCanvas::kStrict_SrcRectConstraint);
  }

  void drawChar(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, int chr) override {
    CommonFont* commonFont = static_cast<CommonFont*>(font);

    gfx::Rect charBounds = commonFont->getCharBounds(chr);
    if (!charBounds.isEmpty()) {
      ScopedSurfaceLock lock(commonFont->getSurfaceSheet());
      drawColoredRgbaSurface(lock, fg, bg, gfx::Clip(x, y, charBounds));
    }
  }

  void drawString(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, const std::string& str) override {
    base::utf8_const_iterator it(str.begin()), end(str.end());
    while (it != end) {
      drawChar(font, fg, bg, x, y, *it);
      x += font->charWidth(*it);
      ++it;
    }
  }

  SkBitmap& bitmap() {
    return m_bitmap;
  }

  void swapBitmap(SkBitmap& other) {
    ASSERT(!m_surface);

    m_bitmap.swap(other);
    rebuild();
  }

private:
  void rebuild() {
    ASSERT(!m_surface);

    delete m_canvas;
    m_canvas = new SkCanvas(m_bitmap);
    m_clip = gfx::Rect(0, 0, width(), height());
  }

  SkBitmap m_bitmap;
  SkSurface* m_surface;
  SkCanvas* m_canvas;
  gfx::Rect m_clip;
};

} // namespace she

#endif
