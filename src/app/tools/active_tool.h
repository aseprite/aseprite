// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_ACTIVE_TOOL_H_INCLUDED
#define APP_TOOLS_ACTIVE_TOOL_H_INCLUDED
#pragma once

#include "app/tools/ink_type.h"
#include "obs/observable.h"

namespace app {
class Color;

namespace tools {

class ActiveToolObserver;
class Ink;
class Pointer;
class Tool;
class ToolBox;

// Manages the coordination between different UI elements that show
// information about the active tool.
class ActiveToolManager : public obs::observable<ActiveToolObserver> {
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
  void brushChanged();
  void regularTipProximity();
  void eraserTipProximity();
  void pressButton(const Pointer& pointer);
  void releaseButtons();
  void setSelectedTool(Tool* tool);

  Ink* adjustToolInkDependingOnSelectedInkType(Ink* ink,
                                               const InkType inkType,
                                               const app::Color& color) const;

  void setAllowQuickToolChanges(bool state);

private:
  static bool isToolAffectedByRightClickMode(Tool* tool);

  ToolBox* m_toolbox;

  // Quick tool in the active sprite editor (activated by keyboard
  // shortuts).
  Tool* m_quickTool;

  // This can be disable temporarily in case we want to use an Editor
  // in "preview only mode" (e.g. when we're previewing a filter we
  // want to disable the Ctrl key to change to the Move tool).
  bool m_allowQuickToolChanges;

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
