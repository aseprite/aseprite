// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_ACTIVE_TOOL_OBSERVER_H_INCLUDED
#define APP_TOOLS_ACTIVE_TOOL_OBSERVER_H_INCLUDED
#pragma once

namespace app { namespace tools {

class Tool;

class ActiveToolObserver {
public:
  virtual ~ActiveToolObserver() {}

  // Called when a new tool is active.
  virtual void onActiveToolChange(tools::Tool* tool) {}

  // Called when a new tool is selected in the tool box.
  virtual void onSelectedToolChange(tools::Tool* tool) {}
};

}} // namespace app::tools

#endif
