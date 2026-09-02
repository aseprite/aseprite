// Aseprite UI Library
// Copyright (c) 2026-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/devmode.h"

namespace ui {

// MoveRegionOnViewScroll is enabled by default
static DevModeFlags g_devmode_flags = DevModeFlags::MoveRegionOnViewScroll;

void set_devmode_flags(const DevModeFlags flags)
{
  g_devmode_flags = flags;
}

DevModeFlags get_devmode_flags()
{
  return g_devmode_flags;
}

} // namespace ui
