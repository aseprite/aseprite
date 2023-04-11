// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_KEY_CONTEXT_H_INCLUDED
#define APP_UI_KEY_CONTEXT_H_INCLUDED
#pragma once

namespace app {

  enum class KeyContext {
    Any,
    Normal,
    SelectionTool,
    TranslatingSelection,
    ScalingSelection,
    RotatingSelection,
    MoveTool,
    FreehandTool,
    ShapeTool,
    MouseWheel,
    FramesSelection,
  };

} // namespace app

#endif
