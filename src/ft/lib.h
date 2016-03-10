// Aseprite FreeType Wrapper
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_LIB_H_INCLUDED
#define FT_LIB_H_INCLUDED
#pragma once

#include "ft/freetype_headers.h"

namespace ft {

  class Lib {
  public:
    Lib() : m_ft(nullptr) {
      FT_Init_FreeType(&m_ft);
    }

    ~Lib() {
      FT_Done_FreeType(m_ft);
    }

    operator FT_Library() {
      return m_ft;
    }

    FT_Face open(const std::string& filename) {
      FT_Open_Args args;
      memset(&args, 0, sizeof(args));
      args.flags = FT_OPEN_PATHNAME;
      args.pathname = (FT_String*)filename.c_str();

      FT_Face face = nullptr;
      FT_Error err = FT_Open_Face(m_ft, &args, 0, &face);
      return face;
    }

  private:
    FT_Library m_ft;
  };

} // namespace ft

#endif
