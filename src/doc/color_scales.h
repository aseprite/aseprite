// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_COLOR_SCALES_H_INCLUDED
#define DOC_COLOR_SCALES_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/frame.h"
#include "doc/object.h"

#include <vector>

namespace doc {

  inline int scale_2bits_to_8bits(int channel2bits) {
    static int scale[4] = {
      0,   85,  170,  255
    };
    ASSERT(channel2bits >= 0);
    ASSERT(channel2bits < 4);
    return scale[channel2bits];
  }

  inline int scale_3bits_to_8bits(int channel3bits) {
    static int scale[8] = {
      0,   36,  72,  109,
      145, 182, 218, 255
    };
    ASSERT(channel3bits >= 0);
    ASSERT(channel3bits < 8);
    return scale[channel3bits];
  }

  inline int scale_4bits_to_8bits(int channel4bits) {
    static int scale[16] = {
      0,   16,  34,  51,
      68,  85,  102, 119,
      136, 153, 170, 187,
      204, 221, 238, 255
    };
    ASSERT(channel4bits >= 0);
    ASSERT(channel4bits < 16);
    return scale[channel4bits];
  }

  inline int scale_5bits_to_8bits(int channel5bits) {
    static int scale[32] = {
      0,   8,   16,  24,  33,  41,  49,  57,
      66,  74,  82,  90,  99,  107, 115, 123,
      132, 140, 148, 156, 165, 173, 181, 189,
      198, 206, 214, 222, 231, 239, 247, 255
    };
    ASSERT(channel5bits >= 0);
    ASSERT(channel5bits < 32);
    return scale[channel5bits];
  }

  inline int scale_6bits_to_8bits(int channel6bits) {
    static int scale[64] = {
      0,   4,   8,   12,  16,  20,  24,  28,
      32,  36,  40,  44,  48,  52,  56,  60,
      65,  69,  73,  77,  81,  85,  89,  93,
      97,  101, 105, 109, 113, 117, 121, 125,
      130, 134, 138, 142, 146, 150, 154, 158,
      162, 166, 170, 174, 178, 182, 186, 190,
      195, 199, 203, 207, 211, 215, 219, 223,
      227, 231, 235, 239, 243, 247, 251, 255
    };
    ASSERT(channel6bits >= 0);
    ASSERT(channel6bits < 64);
    return scale[channel6bits];
  }

} // namespace doc

#endif
