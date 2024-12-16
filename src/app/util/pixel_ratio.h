// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_PIXEL_RATIO_H_INCLUDED
#define APP_UTIL_PIXEL_RATIO_H_INCLUDED
#pragma once

#include "base/convert_to.h"
#include "doc/pixel_ratio.h"

namespace base {

template<>
doc::PixelRatio convert_to(const std::string& from);
template<>
std::string convert_to(const doc::PixelRatio& from);

} // namespace base

#endif
