// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#define APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "gfx/fwd.h"

namespace doc {
  class Image;
}

namespace app {
  class Context;
  class Editor;

  namespace tools {
    class ToolLoop;
  }

  tools::ToolLoop* create_tool_loop(
    Editor* editor,
    Context* context,
    const bool convertLineToFreehand);

  tools::ToolLoop* create_tool_loop_preview(
    Editor* editor,
    doc::Image* image,
    const gfx::Point& celOrigin);

} // namespace app

#endif
