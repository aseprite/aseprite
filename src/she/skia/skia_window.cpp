// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_window.h"

#include "she/skia/skia_display.h"

namespace she {

SkiaWindow::SkiaWindow(SkiaEventQueue* queue, SkiaDisplay* display)
  : m_queue(queue)
  , m_display(display)
{
}

void SkiaWindow::queueEventImpl(Event& ev)
{
  ev.setDisplay(m_display);
  m_queue->queueEvent(ev);
}

void SkiaWindow::paintImpl(HDC hdc)
{
  SkiaSurface* surface = static_cast<SkiaSurface*>(m_display->getSurface());
  const SkBitmap& bitmap = surface->bitmap();

  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = bitmap.width();
  bmi.bmiHeader.biHeight = -bitmap.height();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  ASSERT(bitmap.width() * bitmap.bytesPerPixel() == bitmap.rowBytes());
  bitmap.lockPixels();

  int ret = StretchDIBits(hdc,
    0, 0, bitmap.width()*scale(), bitmap.height()*scale(),
    0, 0, bitmap.width(), bitmap.height(),
    bitmap.getPixels(),
    &bmi, DIB_RGB_COLORS, SRCCOPY);
  (void)ret;

  bitmap.unlockPixels();
}

void SkiaWindow::resizeImpl(const gfx::Size& size)
{
  m_display->resize(size);
}

} // namespace she
