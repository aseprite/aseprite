// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#define APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/ink_type.h"
#include "app/tools/pointer.h"
#include "app/tools/tool_loop.h"
#include "doc/brush.h"
#include "doc/image_ref.h"
#include "gfx/fwd.h"

namespace doc {
  class Image;
}

namespace app {
  class Color;
  class Context;
  class Editor;
  class Site;

  struct ToolLoopParams {
    tools::Tool* tool = nullptr;
    tools::Ink* ink = nullptr;
    tools::Controller* controller = nullptr;

    tools::ToolLoop::Button button = tools::ToolLoop::Left;
    tools::InkType inkType = tools::InkType::DEFAULT;
    app::Color fg;
    app::Color bg;
    doc::BrushRef brush;

    // Options equal to (and with the same default as in)
    // <preferences><tool>...</tool></preferences> from
    // "data/pref.xml" file.
    int opacity = 255;
    int tolerance = 0;
    bool contiguous = true;
    tools::FreehandAlgorithm freehandAlgorithm = tools::FreehandAlgorithm::DEFAULT;
  };

  //////////////////////////////////////////////////////////////////////
  // For UI

#ifdef ENABLE_UI

  tools::ToolLoop* create_tool_loop(
    Editor* editor,
    Context* context,
    const tools::Pointer::Button button,
    const bool convertLineToFreehand,
    const bool selectTiles);

  tools::ToolLoop* create_tool_loop_preview(
    Editor* editor,
    const doc::BrushRef& brush,
    doc::Image* image,
    const gfx::Point& celOrigin);

#endif // ENABLE_UI

  //////////////////////////////////////////////////////////////////////
  // For scripting

#ifdef ENABLE_SCRIPTING

  tools::ToolLoop* create_tool_loop_for_script(
    Context* context,
    const Site& site,
    ToolLoopParams& params);

#endif // ENABLE_SCRIPTING

} // namespace app

#endif
