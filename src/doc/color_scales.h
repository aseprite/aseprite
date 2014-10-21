// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_COLOR_SCALES_H_INCLUDED
#define DOC_COLOR_SCALES_H_INCLUDED
#pragma once

#include "doc/frame_number.h"
#include "doc/object.h"

#include <vector>

namespace doc {

  int scale_5bits_to_8bits(int channel5bits);
  int scale_6bits_to_8bits(int channel6bits);

} // namespace doc

#endif
