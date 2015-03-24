// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaSystem : public CommonSystem {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr) {
  }

  ~SkiaSystem() {
  }

  void dispose() override {
    delete this;
  }

  Capabilities capabilities() const override {
    return Capabilities(
      int(kMultipleDisplaysCapability) |
      int(kCanResizeDisplayCapability) |
      int(kDisplayScaleCapability));
  }

  Display* defaultDisplay() override {
    return m_defaultDisplay;
  }

  Display* createDisplay(int width, int height, int scale) override {
    SkiaDisplay* display = new SkiaDisplay(width, height, scale);
    if (!m_defaultDisplay)
      m_defaultDisplay = display;
    return display;
  }

  Surface* createSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->create(width, height);
    return sur;
  }

  Surface* createRgbaSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->createRgba(width, height);
    return sur;
  }

  Surface* loadSurface(const char* filename) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->create(32, 32);
    return sur;
  }

  Surface* loadRgbaSurface(const char* filename) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->create(32, 32);
    return sur;
  }

private:
  SkiaDisplay* m_defaultDisplay;
};

} // namespace she
