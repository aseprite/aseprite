// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_FONT_H_INCLUDED
#define SHE_FONT_H_INCLUDED
#pragma once

namespace she {

  class Font {
  public:
    virtual ~Font() { }
    virtual void dispose() = 0;
    virtual int height() const = 0;
    virtual int charWidth(int chr) const = 0;
    virtual int textLength(const char* chr) const = 0;
    virtual bool isScalable() const = 0;
    virtual void setSize(int size) = 0;
    virtual void* nativeHandle() = 0;
  };

  Font* load_bitmap_font(const char* filename, int scale = 1);

} // namespace she

#endif
