// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_OPTIONS_BEHAVIOR_H_INCLUDED
#define APP_COMMANDS_OPTIONS_BEHAVIOR_H_INCLUDED
#pragma once

#include "ui/widget.h"

namespace app {

inline void update_pixel_grid_options_enabled(const bool state,
                                              ui::Widget* pixelGridColorLabel,
                                              ui::Widget* pixelGridColor,
                                              ui::Widget* pixelGridOpacityLabel,
                                              ui::Widget* pixelGridOpacity,
                                              ui::Widget* pixelGridAutoOpacity)
{
  pixelGridColorLabel->setEnabled(state);
  pixelGridColor->setEnabled(state);
  pixelGridOpacityLabel->setEnabled(state);
  pixelGridOpacity->setEnabled(state);
  pixelGridAutoOpacity->setEnabled(state);
}

} // namespace app

#endif
