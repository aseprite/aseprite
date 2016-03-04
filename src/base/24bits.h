// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_24BITS_H_INCLUDED
#define BASE_24BITS_H_INCLUDED
#pragma once

#include "base/config.h"
#include "base/ints.h"

namespace base {

#ifdef ASEPRITE_LITTLE_ENDIAN

  template<typename PTR, typename VALUE>
  inline void write24bits(PTR* ptr, VALUE value) {
    ((uint8_t*)ptr)[0] = value;
    ((uint8_t*)ptr)[1] = value >> 8;
    ((uint8_t*)ptr)[2] = value >> 16;
  }

#elif defined(ASEPRITE_BIG_ENDIAN)

  template<typename PTR, typename VALUE>
  inline void write24bits(PTR* ptr, VALUE value) {
    ((uint8_t*)ptr)[0] = value >> 16;
    ((uint8_t*)ptr)[1] = value >> 8;
    ((uint8_t*)ptr)[2] = value;
  }

#endif

} // namespace base

#endif
