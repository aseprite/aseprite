// Aseprite FreeType Wrapper
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_LIB_H_INCLUDED
#define FT_LIB_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "ft/freetype_headers.h"

#include <string>

namespace ft {

  class Lib {
  public:
    Lib();
    ~Lib();

    operator FT_Library() { return m_ft; }

    FT_Face open(const std::string& filename);

  private:
    FT_Library m_ft;

    DISABLE_COPYING(Lib);
  };

} // namespace ft

#endif
