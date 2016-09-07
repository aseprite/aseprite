// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_HEX_H_INCLUDED
#define BASE_HEX_H_INCLUDED
#pragma once

namespace base {

  inline bool is_hex_digit(int digit) {
    return ((digit >= '0' && digit <= '9') ||
            (digit >= 'a' && digit <= 'f') ||
            (digit >= 'A' && digit <= 'F'));
  }

} // namespace base

#endif
