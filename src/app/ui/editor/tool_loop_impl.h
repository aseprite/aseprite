// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
    Editor* editor, Context* context);

  tools::ToolLoop* create_tool_loop_preview(
    Editor* editor, doc::Image* image,
    const gfx::Point& celOrigin);

} // namespace app

#endif
