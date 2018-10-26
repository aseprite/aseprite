// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_LOOP_MODIFIERS_H_INCLUDED
#define APP_TOOLS_TOOL_LOOP_MODIFIERS_H_INCLUDED
#pragma once

namespace app {
namespace tools {

  enum class ToolLoopModifiers {
    kNone               = 0x00000000,
    kReplaceSelection   = 0x00000001,
    kAddSelection       = 0x00000002,
    kSubtractSelection  = 0x00000004,
    kIntersectSelection = 0x00000008,
    kMoveOrigin         = 0x00000010,
    kSquareAspect       = 0x00000020,
    kFromCenter         = 0x00000040,
    kRotateShape        = 0x00000080,
  };

} // namespace tools
} // namespace app

#endif
