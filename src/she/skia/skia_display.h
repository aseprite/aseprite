// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace she {

class SkiaDisplay : public Display {
public:
  SkiaDisplay(int width, int height, int scale)
    : m_window(&m_queue, this)
    , m_scale(scale) {
    m_surface.create(width, height);
    m_window.setVisible(true);
  }

  void dispose() override {
    delete this;
  }

  // Returns the real and current display's size (without scale applied).
  int width() const override {
    return m_window.clientSize().w;
  }

  int height() const override {
    return m_window.clientSize().h;
  }

  // Returns the display when it was not maximized.
  int originalWidth() const override {
    return m_window.restoredSize().w;
  }

  int originalHeight() const override {
    return m_window.restoredSize().h;
  }

  void setScale(int scale) override {
    m_scale = scale;
  }

  int scale() const override {
    return m_scale;
  }

  // Returns the main surface to draw into this display.
  // You must not dispose this surface.
  NonDisposableSurface* getSurface() override {
    return static_cast<NonDisposableSurface*>(&m_surface);
  }

  // Flips all graphics in the surface to the real display.  Returns
  // false if the flip couldn't be done because the display was
  // resized.
  bool flip() override {
    return true;
  }

  void maximize() override {
    m_window.maximize();
  }

  bool isMaximized() const override {
    return m_window.isMaximized();
  }

  void setTitleBar(const std::string& title) override {
    m_window.setText(title);
  }

  EventQueue* getEventQueue() override {
    return &m_queue;
  }

  bool setNativeMouseCursor(NativeCursor cursor) override {
    return true;
  }

  void setMousePosition(const gfx::Point& position) override {
  }

  void captureMouse() override {
    m_window.captureMouse();
  }

  void releaseMouse() override {
    m_window.releaseMouse();
  }

  // Returns the HWND on Windows.
  DisplayHandle nativeHandle() override {
    return (DisplayHandle)m_window.handle();
  }

private:
  SkiaEventQueue m_queue;
  SkiaWindow m_window;
  SkiaSurface m_surface;
  int m_scale;
};

} // namespace she
