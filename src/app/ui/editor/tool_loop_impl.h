// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#define APP_UI_EDITOR_TOOL_LOOP_IMPL_H_INCLUDED
#pragma once

namespace app {
  class Context;
  class Editor;

  namespace tools {
    class ToolLoop;
  }

  tools::ToolLoop* create_tool_loop(Editor* editor, Context* context);

} // namespace app

#endif
