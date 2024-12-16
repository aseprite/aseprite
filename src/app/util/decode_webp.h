// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_DECODE_WEBP_H_INCLUDED
#define APP_UTIL_DECODE_WEBP_H_INCLUDED
#pragma once

#include "os/surface.h"

namespace app { namespace util {

// Decodes webp content passed in buf and returns a surface with just the first
// frame.
os::SurfaceRef decode_webp(const uint8_t* buf, uint32_t len);

}} // namespace app::util

#endif
