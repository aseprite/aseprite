// SHE library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_COMMON_FREETYPE_FONT_H_INCLUDED
#define SHE_COMMON_FREETYPE_FONT_H_INCLUDED
#pragma once

#include "ft/hb_face.h"
#include "ft/lib.h"
#include "she/font.h"

namespace she {
  class Font;

  class FreeTypeFont : public Font {
  public:
    typedef ft::Face Face;

    FreeTypeFont(const char* filename, int height);
    ~FreeTypeFont();

    bool isValid() const;
    void dispose() override;
    FontType type() override;
    int height() const override;
    int textLength(const std::string& str) const override;
    bool isScalable() const override;
    void setSize(int size) override;
    void setAntialias(bool antialias) override;
    bool hasCodePoint(int codepoint) const override;

    Face& face() { return m_face; }

  private:
    mutable ft::Lib m_ft;
    mutable Face m_face;
  };

  FreeTypeFont* loadFreeTypeFont(const char* filename, int height);

} // namespace she

#endif
