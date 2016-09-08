// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_RECT_IO_H_INCLUDED
#define GFX_RECT_IO_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include <iosfwd>

namespace gfx {

  inline std::ostream& operator<<(std::ostream& os, const Rect& rect) {
    return os << "("
              << rect.x << ", "
              << rect.y << ", "
              << rect.w << ", "
              << rect.h << ")";
  }

  inline std::istream& operator>>(std::istream& in, Rect& rect) {
    while (in && in.get() != '(')
      ;

    if (!in)
      return in;

    char chr;
    in >> rect.x >> chr
       >> rect.y >> chr
       >> rect.w >> chr
       >> rect.h >> chr;

    return in;
  }

}

#endif
