// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_CLAMP_H_INCLUDED
#define BASE_CLAMP_H_INCLUDED
#pragma once

namespace base {

  template<typename T>
  T clamp(const T& value, const T& low, const T& high) {
    return (value > high ? high:
            (value < low ? low:
                           value));
  }

} // namespace base

#endif
