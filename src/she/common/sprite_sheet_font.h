// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SPRITE_SHEET_FONT_H
#define SHE_SPRITE_SHEET_FONT_H
#pragma once

#include "base/debug.h"
#include "base/string.h"
#include "gfx/rect.h"
#include "she/font.h"
#include "she/surface.h"

#include <vector>

namespace she {

class SpriteSheetFont : public Font {
public:

  SpriteSheetFont() : m_sheet(nullptr) {
  }

  ~SpriteSheetFont() {
    ASSERT(m_sheet);
    m_sheet->dispose();
  }

  void dispose() override {
    delete this;
  }

  FontType type() override {
    return FontType::kSpriteSheet;
  }

  int height() const override {
    return getCharBounds(' ').h;
  }

  int textLength(const std::string& str) const override {
    base::utf8_const_iterator it(str.begin()), end(str.end());
    int x = 0;
    while (it != end) {
      x += getCharBounds(*it).w;
      ++it;
    }
    return x;
  }

  bool isScalable() const override {
    return false;
  }

  void setSize(int size) override {
    // Do nothing
  }

  void setAntialias(bool antialias) override {
    // Do nothing
  }

  bool hasCodePoint(int codepoint) const override {
    codepoint -= (int)' ';
    return (codepoint >= 0 && codepoint < (int)m_chars.size());
  }

  Surface* getSurfaceSheet() const {
    return m_sheet;
  }

  gfx::Rect getCharBounds(int chr) const {
    chr -= (int)' ';
    if (chr >= 0 && chr < (int)m_chars.size())
      return m_chars[chr];
    else
      return gfx::Rect();
  }

  static Font* fromSurface(Surface* sur) {
    SpriteSheetFont* font = new SpriteSheetFont;
    font->m_sheet = sur;

    SurfaceLock lock(sur);
    gfx::Rect bounds(0, 0, 1, 1);

    while (font->findChar(sur, sur->width(), sur->height(), bounds)) {
      font->m_chars.push_back(bounds);
      bounds.x += bounds.w;
    }

    return font;
  }

private:

  bool findChar(const Surface* sur, int width, int height, gfx::Rect& bounds) {
    gfx::Color keyColor = sur->getPixel(0, 0);

    while (sur->getPixel(bounds.x, bounds.y) == keyColor) {
      bounds.x++;
      if (bounds.x >= width) {
        bounds.x = 0;
        bounds.y += bounds.h;
        bounds.h = 1;
        if (bounds.y >= height)
          return false;
      }
    }

    bounds.w = 0;
    while ((bounds.x+bounds.w < width) &&
           (sur->getPixel(bounds.x+bounds.w, bounds.y) != keyColor)) {
      bounds.w++;
    }

    bounds.h = 0;
    while ((bounds.y+bounds.h < height) &&
           (sur->getPixel(bounds.x, bounds.y+bounds.h) != keyColor)) {
      bounds.h++;
    }

    return !bounds.isEmpty();
  }

private:
  Surface* m_sheet;
  std::vector<gfx::Rect> m_chars;
};

} // namespace she

#endif
