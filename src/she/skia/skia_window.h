// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaWindow : public Window<SkiaWindow> {
public:
  SkiaWindow(SkiaEventQueue* queue, Display* display)
    : m_queue(queue)
    , m_display(display) {
  }

  void queueEventImpl(Event& ev) {
    ev.setDisplay(m_display);
    m_queue->queueEvent(ev);
  }

  void paintImpl(HDC hdc) {
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

private:
  SkiaEventQueue* m_queue;
  Display* m_display;
};

} // namespace she
