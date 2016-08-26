// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WRAP_VALUE_H_INCLUDED
#define APP_WRAP_VALUE_H_INCLUDED
#pragma once

namespace app {

  template<typename T>
  inline T wrap_value(const T x, const T size) {
    if (x < T(0))
      return size - (-(x+1) % size) - 1;
    else
      return x % size;
  }

} // namespace app

#endif
