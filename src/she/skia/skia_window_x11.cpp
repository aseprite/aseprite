// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_window_x11.h"

#include "gfx/size.h"
#include "she/event.h"
#include "she/event_queue.h"
#include "she/skia/skia_display.h"
#include "she/skia/skia_surface.h"
#include "she/x11/x11.h"

#include "SkBitmap.h"

namespace she {

namespace {

bool convert_skia_bitmap_to_ximage(const SkBitmap& bitmap, XImage& image)
{
  memset(&image, 0, sizeof(image));
  int bpp = 8*bitmap.bytesPerPixel();
  image.width = bitmap.width();
  image.height = bitmap.height();
  image.format = ZPixmap;
  image.data = (char*)bitmap.getPixels();
  image.byte_order = LSBFirst;
  image.bitmap_unit = bpp;
  image.bitmap_bit_order = LSBFirst;
  image.bitmap_pad = bpp;
  image.depth = 24;
  image.bytes_per_line = bitmap.rowBytes() - 4*bitmap.width();
  image.bits_per_pixel = bpp;

  bool result = XInitImage(&image);

  return result;
}

} // anonymous namespace

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display,
                       int width, int height, int scale)
  : X11Window(X11::instance()->display(), width, height, scale)
  , m_queue(queue)
  , m_display(display)
{
}

SkiaWindow::~SkiaWindow()
{
}

void SkiaWindow::setVisible(bool visible)
{
  // TODO
}

void SkiaWindow::maximize()
{
  // TODO
}

bool SkiaWindow::isMaximized() const
{
  return false;
}

bool SkiaWindow::isMinimized() const
{
  return false;
}

void SkiaWindow::queueEvent(Event& ev)
{
  ev.setDisplay(m_display);
  m_queue->queueEvent(ev);
}

void SkiaWindow::paintGC(const gfx::Rect& rc)
{
  SkiaSurface* surface = static_cast<SkiaSurface*>(m_display->getSurface());
  const SkBitmap& bitmap = surface->bitmap();

  int scale = this->scale();
  if (scale == 1) {
    XImage image;
    if (convert_skia_bitmap_to_ximage(bitmap, image)) {
      XPutImage(
        x11display(), handle(), gc(), &image,
        rc.x, rc.y,
        rc.x, rc.y,
        rc.w, rc.h);
    }
  }
  else {
    SkBitmap scaled;
    if (scaled.tryAllocPixels(
          SkImageInfo::Make(rc.w, rc.h,
                            bitmap.info().colorType(),
                            bitmap.info().alphaType()))) {
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);

      SkCanvas canvas(scaled);
      SkRect srcRect = SkRect::Make(SkIRect::MakeXYWH(rc.x/scale, rc.y/scale, rc.w/scale, rc.h/scale));
      SkRect dstRect = SkRect::Make(SkIRect::MakeXYWH(0, 0, rc.w, rc.h));
      canvas.drawBitmapRect(bitmap, srcRect, dstRect, &paint,
                            SkCanvas::kStrict_SrcRectConstraint);

      XImage image;
      if (convert_skia_bitmap_to_ximage(scaled, image)) {
        XPutImage(
          x11display(), handle(), gc(), &image,
          0, 0,
          rc.x, rc.y,
          rc.w, rc.h);
      }
    }
  }
}

void SkiaWindow::resizeDisplay(const gfx::Size& sz)
{
  m_display->resize(sz);
  updateWindow(gfx::Rect(sz / scale()));
}

} // namespace she
