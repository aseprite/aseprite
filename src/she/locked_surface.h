// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_LOCKED_SURFACE_H_INCLUDED
#define SHE_LOCKED_SURFACE_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "gfx/fwd.h"
#include "she/surface_format.h"

#include <string>

namespace she {

  class Font;

  class LockedSurface {
  public:
    virtual ~LockedSurface() { }
    virtual void unlock() = 0;
    virtual void clear() = 0;

    virtual uint8_t* getData(int x, int y) = 0;
    virtual void getFormat(SurfaceFormatData* formatData) = 0;

    virtual gfx::Color getPixel(int x, int y) = 0;
    virtual void putPixel(gfx::Color color, int x, int y) = 0;

    virtual void drawHLine(gfx::Color color, int x, int y, int w) = 0;
    virtual void drawVLine(gfx::Color color, int x, int y, int h) = 0;
    virtual void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) = 0;

    virtual void drawRect(gfx::Color color, const gfx::Rect& rc) = 0;
    virtual void fillRect(gfx::Color color, const gfx::Rect& rc) = 0;

    virtual void blitTo(LockedSurface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const = 0;
    virtual void drawSurface(const LockedSurface* src, int dstx, int dsty) = 0;
    virtual void drawRgbaSurface(const LockedSurface* src, int dstx, int dsty) = 0;

    virtual void drawChar(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, int chr) = 0;
    virtual void drawString(Font* font, gfx::Color fg, gfx::Color bg, int x, int y, const std::string& str) = 0;
  };

} // namespace she

#endif
