// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_COORD_H_INCLUDED
#define APP_UI_EDITOR_COORD_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace app {

enum class EditorCoord : uint8_t {
  // Coordinate without applying the ui::Graphics transformation, so
  // the coordinate can be painted. The coordinate is relative to the
  // Editor origin.
  BeforePaint = 0,

  // Coordinate on screen after applying the ui::Graphics
  // transformation (so the coordinate is used as mouse/screen)
  OnScreen = 1,
};

} // namespace app

#endif
