// Aseprite UI Library
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_DEVMODE_H_INCLUDED
#define UI_DEVMODE_H_INCLUDED
#pragma once

#include "base/enum_flags.h"
#include "base/ints.h"

namespace ui {

enum class DevModeFlags : uint8_t {
  None = 0,
  DebugPaint = 1,
  PaintBaseline = 2,
  DebugViewScroll = 4,
  MoveRegionOnViewScroll = 8,
};

LAF_ENUM_FLAGS(DevModeFlags);

void set_devmode_flags(DevModeFlags flags);
DevModeFlags get_devmode_flags();

inline bool has_devmode_flags(const DevModeFlags flags)
{
  return (get_devmode_flags() & flags) == flags;
}

} // namespace ui

#endif
