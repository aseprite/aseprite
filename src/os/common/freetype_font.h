// LAF OS Library
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_COMMON_FREETYPE_FONT_H_INCLUDED
#define OS_COMMON_FREETYPE_FONT_H_INCLUDED
#pragma once

#include "ft/hb_face.h"
#include "ft/lib.h"
#include "os/font.h"

namespace os {
  class Font;

  class FreeTypeFont : public Font {
  public:
    typedef ft::Face Face;

    FreeTypeFont(ft::Lib& lib,
                 const char* filename,
                 const int height);
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
    mutable Face m_face;
  };

  FreeTypeFont* load_free_type_font(ft::Lib& lib,
                                    const char* filename,
                                    const int height);

} // namespace os

#endif
