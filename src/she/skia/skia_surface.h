// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaSurface : public NonDisposableSurface
                  , public LockedSurface {
public:
  SkiaSurface() {
  }

  void create(int width, int height) {
    m_bitmap.tryAllocPixels(
      SkImageInfo::MakeUnknown(width, height));
  }

  void createRgba(int width, int height) {
    m_bitmap.tryAllocPixels(
      SkImageInfo::MakeN32(width, height, kUnpremul_SkAlphaType));
  }

  // Surface impl

  void dispose() override {
    delete this;
  }

  int width() const override {
    return m_bitmap.width();
  }

  int height() const override {
    return m_bitmap.height();
  }

  bool isDirectToScreen() const override {
    return false;
  }

  gfx::Rect getClipBounds() override {
    return gfx::Rect();
  }

  void setClipBounds(const gfx::Rect& rc) override {
  }

  bool intersectClipRect(const gfx::Rect& rc) override {
    return true;
  }

  void setDrawMode(DrawMode mode, int param) {
  }

  LockedSurface* lock() override {
    m_bitmap.lockPixels();
    return this;
  }

  void applyScale(int scaleFactor) override {
  }

  void* nativeHandle() override {
    return (void*)this;
  }

  // LockedSurface impl

  void unlock() override {
    m_bitmap.unlockPixels();
  }

  void clear() override {
  }

  uint8_t* getData(int x, int y) override {
    return (uint8_t*)m_bitmap.getAddr(x, y);
  }

  void getFormat(SurfaceFormatData* formatData) override {
  }

  gfx::Color getPixel(int x, int y) override {
    SkColor c = m_bitmap.getColor(x, y);
    return gfx::rgba(
      SkColorGetR(c),
      SkColorGetG(c),
      SkColorGetB(c),
      SkColorGetA(c));
  }

  void putPixel(gfx::Color color, int x, int y) override {
  }

  void drawHLine(gfx::Color color, int x, int y, int w) override {
  }

  void drawVLine(gfx::Color color, int x, int y, int h) override {
  }

  void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) override {
  }

  void drawRect(gfx::Color color, const gfx::Rect& rc) override {
  }

  void fillRect(gfx::Color color, const gfx::Rect& rc) override {
  }

  void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const override {
  }

  void drawSurface(const LockedSurface* src, int dstx, int dsty) override {
  }

  void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) override {
  }

  void drawChar(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, int chr) override {
  }

  void drawString(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, const std::string& str) override {
  }

  SkBitmap& bitmap() {
    return m_bitmap;
  }

private:
  SkBitmap m_bitmap;
};

} // namespace she
