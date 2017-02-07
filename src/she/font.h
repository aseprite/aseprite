// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_FONT_H_INCLUDED
#define SHE_FONT_H_INCLUDED
#pragma once

#include <string>

namespace she {

  enum class FontType {
    kUnknown,
    kSpriteSheet,
    kTrueType,
  };

  class Font {
  public:
    virtual ~Font() { }
    virtual void dispose() = 0;
    virtual FontType type() = 0;
    virtual int height() const = 0;
    virtual int textLength(const std::string& str) const = 0;
    virtual bool isScalable() const = 0;
    virtual void setSize(int size) = 0;
    virtual void setAntialias(bool antialias) = 0;
  };

} // namespace she

#endif
