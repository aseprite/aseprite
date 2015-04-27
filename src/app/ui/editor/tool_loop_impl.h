// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#define APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "doc/image_ref.h"
#include "gfx/fwd.h"

namespace doc {
  class Brush;
  class Image;
}

namespace app {
  class Context;
  class Editor;
  class IBrushSettings;

  namespace tools {
    class ToolLoop;
  }

  void set_tool_loop_brush_image(
    const doc::Image* image,
    const gfx::Point& origin);
  bool is_tool_loop_brush_image();
  void discard_tool_loop_brush_image();
  doc::Image* get_tool_loop_brush_image();

  base::SharedPtr<doc::Brush> get_tool_loop_brush(
    IBrushSettings* brushSettings);

  tools::ToolLoop* create_tool_loop(
    Editor* editor, Context* context);

  tools::ToolLoop* create_tool_loop_preview(
    Editor* editor, Context* context, doc::Image* image,
    const gfx::Point& offset);

} // namespace app

#endif
