// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SURFACE_H_INCLUDED
#define SHE_SURFACE_H_INCLUDED
#pragma once

#include "base/string.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "she/surface_format.h"

#include <string>

namespace she {

  class SurfaceLock;

  enum class DrawMode {
    Solid,
    Checked,
    Xor
  };

  class Surface {
  public:
    virtual ~Surface() { }
    virtual void dispose() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual bool isDirectToScreen() const = 0;

    virtual int getSaveCount() const = 0;
    virtual gfx::Rect getClipBounds() const = 0;
    virtual void saveClip() = 0;
    virtual void restoreClip() = 0;
    virtual bool clipRect(const gfx::Rect& rc) = 0;

    virtual void setDrawMode(DrawMode mode, int param = 0,
                             const gfx::Color a = gfx::ColorNone,
                             const gfx::Color b = gfx::ColorNone) = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual void clear() = 0;

    virtual uint8_t* getData(int x, int y) const = 0;
    virtual void getFormat(SurfaceFormatData* formatData) const = 0;

    virtual gfx::Color getPixel(int x, int y) const = 0;
    virtual void putPixel(gfx::Color color, int x, int y) = 0;

    virtual void drawHLine(gfx::Color color, int x, int y, int w) = 0;
    virtual void drawVLine(gfx::Color color, int x, int y, int h) = 0;
    virtual void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) = 0;

    virtual void drawRect(gfx::Color color, const gfx::Rect& rc) = 0;
    virtual void fillRect(gfx::Color color, const gfx::Rect& rc) = 0;

    virtual void blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const = 0;
    virtual void scrollTo(const gfx::Rect& rc, int dx, int dy) = 0;
    virtual void drawSurface(const Surface* src, int dstx, int dsty) = 0;
    virtual void drawRgbaSurface(const Surface* src, int dstx, int dsty) = 0;
    virtual void drawRgbaSurface(const Surface* src, int srcx, int srcy, int dstx, int dsty, int width, int height) = 0;
    virtual void drawColoredRgbaSurface(const Surface* src, gfx::Color fg, gfx::Color bg, const gfx::Clip& clip) = 0;

    virtual void applyScale(int scaleFactor) = 0;
    virtual void* nativeHandle() = 0;
  };

  class SurfaceLock {
  public:
    SurfaceLock(Surface* surface) : m_surface(surface) { m_surface->lock(); }
    ~SurfaceLock() { m_surface->unlock(); }
  private:
    Surface* m_surface;
  };

} // namespace she

#endif
