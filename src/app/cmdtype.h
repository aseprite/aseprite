// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMDTYPE_H_INCLUDED
#define APP_CMDTYPE_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace app {

using cmdtype_t = uint32_t;

constexpr cmdtype_t make_cmdtype(int a, int b, int c, int d)
{
  return ((a << 24) | (b << 16) | (c << 8) | d);
}

} // namespace app

#endif
