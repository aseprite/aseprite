// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_DISPLAY_H_INCLUDED
#define SHE_DISPLAY_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "she/display_handle.h"
#include "she/native_cursor.h"
#include "she/surface_list.h"

#include <string>

namespace she {

  class Surface;

  // A display or window to show graphics.
  class Display {
  public:
    virtual ~Display() { }
    virtual void dispose() = 0;

    // Returns the real and current display's size (without scale applied).
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Returns the display when it was not maximized.
    virtual int originalWidth() const = 0;
    virtual int originalHeight() const = 0;

    // Returns the current display scale. Each pixel in the internal
    // display surface, is represented by SCALExSCALE pixels on the
    // screen.
    virtual int scale() const = 0;

    // Changes the scale.
    // The available surface size will be (Display::width() / scale,
    //                                     Display::height() / scale)
    virtual void setScale(int scale) = 0;

    // Returns the main surface to draw into this display.
    // You must not dispose this surface.
    virtual Surface* getSurface() = 0;

    // Flips all graphics in the surface to the real display.
    virtual void flip(const gfx::Rect& bounds) = 0;

    virtual void maximize() = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isMinimized() const = 0;

    virtual void setTitleBar(const std::string& title) = 0;
    virtual void setIcons(const SurfaceList& icons) = 0;

    virtual NativeCursor nativeMouseCursor() = 0;
    virtual bool setNativeMouseCursor(NativeCursor cursor) = 0;
    virtual bool setNativeMouseCursor(const she::Surface* cursor,
                                      const gfx::Point& focus,
                                      const int scale) = 0;
    virtual void setMousePosition(const gfx::Point& position) = 0;
    virtual void captureMouse() = 0;
    virtual void releaseMouse() = 0;

    // Set/get the specific information to restore the exact same
    // window position (e.g. in the same monitor).
    virtual std::string getLayout() = 0;
    virtual void setLayout(const std::string& layout) = 0;

    // Returns the HWND on Windows.
    virtual DisplayHandle nativeHandle() = 0;
  };

} // namespace she

#endif
