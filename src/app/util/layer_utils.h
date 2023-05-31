// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LAYER_UTILS_H_INCLUDED
#define APP_LAYER_UTILS_H_INCLUDED
#pragma once

namespace app {

  class Editor;

  // True if the active layer is locked (itself or its hierarchy),
  // also, it sends a tip to the user 'Layer ... is locked'
  bool layer_is_locked(Editor* editor);

} // namespace app

#endif
