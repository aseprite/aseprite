// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_ACTIVE_TOOL_H_INCLUDED
#define APP_TOOLS_ACTIVE_TOOL_H_INCLUDED
#pragma once

#include "base/observable.h"

namespace app {
namespace tools {

class ActiveToolObserver;
class Ink;
class Pointer;
class Tool;
class ToolBox;

// Manages the coordination between different UI elements that show
// information about the active tool.
class ActiveToolManager : public base::Observable<ActiveToolObserver> {
public:
  ActiveToolManager(ToolBox* toolbox);

  Tool* activeTool() const;
  Ink* activeInk() const;

  // Returns the quick tool.
  Tool* quickTool() const;

  // Returns the selected tool in the toolbar/box.
  Tool* selectedTool() const;

  // These are different events that came from UI elements and
  // modify the active tool.
  void newToolSelectedInToolBar(Tool* tool);
  void newQuickToolSelectedFromEditor(Tool* tool);
  void regularTipProximity();
  void eraserTipProximity();
  void pressButton(const Pointer& pointer);
  void releaseButtons();
  void setSelectedTool(Tool* tool);

private:
  static bool isToolAffectedByRightClickMode(Tool* tool);

  ToolBox* m_toolbox;

  // Quick tool in the active sprite editor (activated by keyboard
  // shortuts).
  Tool* m_quickTool;

  // Special tool by stylus proximity.
  bool m_rightClick;
  Tool* m_rightClickTool;
  Ink* m_rightClickInk;

  // Special tool by stylus proximity (e.g. eraser).
  Tool* m_proximityTool;

  // Selected tool in the toolbar/toolbox.
  Tool* m_selectedTool;
};

} // namespace tools
} // namespace app

#endif
