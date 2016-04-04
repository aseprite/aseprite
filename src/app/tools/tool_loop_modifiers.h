// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
    kMoveOrigin         = 0x00000008,
    kSquareAspect       = 0x00000010,
    kFromCenter         = 0x00000020,
  };

} // namespace tools
} // namespace app

#endif
