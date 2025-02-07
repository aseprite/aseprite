// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_LOOP_MANAGER_H_INCLUDED
#define APP_TOOLS_TOOL_LOOP_MANAGER_H_INCLUDED
#pragma once

#include "app/tools/dynamics.h"
#include "app/tools/pointer.h"
#include "app/tools/stroke.h"
#include "doc/brush.h"
#include "gfx/point.h"
#include "gfx/region.h"

#include <vector>

namespace gfx {
class Region;
}

namespace app { namespace tools {

class ToolLoop;

// Class to manage the drawing tool (editor <-> tool interface).
//
// The flow is this:
// 1. The user press a mouse button in a Editor widget
// 2. The Editor creates an implementation of ToolLoop and use it
//    with the ToolLoopManager constructor
// 3. The ToolLoopManager is used to call
//    the following methods:
//    - ToolLoopManager::prepareLoop
//    - ToolLoopManager::pressButton
// 4. If the user moves the mouse, the method
//    - ToolLoopManager::movement
//    is called.
// 5. When the user release the mouse:
//    - ToolLoopManager::releaseButton
//
class ToolLoopManager {
public:
  // Contructs a manager for the ToolLoop delegate.
  ToolLoopManager(ToolLoop* toolLoop);
  virtual ~ToolLoopManager();

  // Returns true if the loop was canceled by the user
  bool isCanceled() const;

  // Called when the tool loop must be canceled because another
  // command was executed or the Esc key pressed.
  void cancel();

  // Called when the tool loop ends (this will commit or rollback the
  // tool loop).
  void end();

  // Should be called when the user start a tool-trace (pressing the
  // left or right button for first time in the editor).
  void prepareLoop(const Pointer& pointer);

  // Should be called when the ToolLoop::getModifiers()
  // value was modified (e.g. when the user press/release a key).
  void notifyToolLoopModifiersChange();

  // Should be called each time the user presses a mouse button.
  void pressButton(const Pointer& pointer);

  // Should be called each time the user releases a mouse button.
  //
  // Returns true if the tool-loop should continue, or false
  // if the editor should release the mouse capture.
  bool releaseButton(const Pointer& pointer);

  // Should be called each time the user moves the mouse inside the editor.
  void movement(Pointer pointer);

  // Should be called when Shift+brush tool is used to disable stabilizer
  // on the line preview
  void disableMouseStabilizer();

  const Pointer& lastPointer() const { return m_lastPointer; }

private:
  void doLoopStep(bool lastStep);
  void snapToGrid(Stroke::Pt& pt);
  Stroke::Pt getSpriteStrokePt(const Pointer& pointer);
  bool useDynamics() const;
  void adjustPointWithDynamics(const Pointer& pointer, Stroke::Pt& pt);

  void calculateDirtyArea(const Strokes& strokes);

  ToolLoop* m_toolLoop;
  bool m_canceled;
  Stroke m_stroke;
  Pointer m_lastPointer;
  gfx::Region m_dirtyArea;
  gfx::Region m_nextDirtyArea;
  const int m_brushSize0;
  const int m_brushAngle0;
  DynamicsOptions m_dynamics;
  gfx::PointF m_stabilizerCenter;
};

}} // namespace app::tools

#endif
