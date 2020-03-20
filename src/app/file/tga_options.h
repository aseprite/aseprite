// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_TGA_OPTIONS_H_INCLUDED
#define APP_FILE_TGA_OPTIONS_H_INCLUDED
#pragma once

#include "app/file/format_options.h"

namespace app {

  class TgaOptions : public FormatOptions {
  public:
    int bitsPerPixel() const { return m_bitsPerPixel; }
    bool compress() const { return m_compress; }

    void bitsPerPixel(int bpp) { m_bitsPerPixel = bpp; }
    void compress(bool state) { m_compress = state; }

  private:
    int m_bitsPerPixel = 0;
    bool m_compress = true;
  };

} // namespace app

#endif
