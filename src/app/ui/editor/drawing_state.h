// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#define APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#pragma once

#include "app/tools/pointer.h"
#include "app/tools/velocity.h"
#include "app/ui/editor/delayed_mouse_move.h"
#include "app/ui/editor/standby_state.h"
#include "base/time.h"
#include "obs/connection.h"
#include <memory>

namespace app {
namespace tools {
class ToolLoop;
class ToolLoopManager;
} // namespace tools

class CommandExecutionEvent;

class DrawingState : public StandbyState,
                     DelayedMouseMoveDelegate {
public:
  DrawingState(Editor* editor, tools::ToolLoop* loop, const DrawingType type);
  virtual ~DrawingState();
  virtual void onBeforePopState(Editor* editor) override;
  virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onScrollChange(Editor* editor) override;
  virtual bool onUpdateStatusBar(Editor* editor) override;
  virtual void onExposeSpritePixels(const gfx::Region& rgn) override;

  // Drawing state doesn't require the brush-preview because we are
  // already drawing (viewing the real trace).
  virtual bool requireBrushPreview() override { return false; }

  // Don't show layer edges when we're drawing as the cel
  // position/bounds is modified and the feedback is
  // confusing. (This is the original behavior, maybe in the future
  // we could fix this in other way and show how the edge of the cel
  // is expanded dynamically while we paint.)
  virtual bool allowLayerEdges() override { return false; }

  virtual bool getGridBounds(Editor* editor, gfx::Rect& gridBounds) override;

  void initToolLoop(Editor* editor, const ui::MouseMessage* msg, const tools::Pointer& pointer);

  // Used to disable the current ToolLoopManager's stabilizer
  // when Shift+brush tool is used to paint a line
  void disableMouseStabilizer();

  // Used to send a movement() to the ToolLoopManager when
  // Shift+brush tool is used to paint a line.
  void sendMovementToToolLoop(const tools::Pointer& pointer);

  void notifyToolLoopModifiersChange(Editor* editor);

private:
  void handleMouseMovement();
  bool canInterpretMouseMovementAsJustOneClick();
  bool canExecuteCommands();
  void onBeforeCommandExecution(CommandExecutionEvent& ev);
  void destroyLoopIfCanceled(Editor* editor);
  void destroyLoop(Editor* editor);

  // DelayedMouseMoveDelegate impl
  void onCommitMouseMove(Editor* editor, const gfx::PointF& spritePos) override;

  Editor* m_editor;
  DrawingType m_type;
  DelayedMouseMove m_delayedMouseMove;

  // The tool-loop.
  std::unique_ptr<tools::ToolLoop> m_toolLoop;

  // Tool-loop manager
  std::unique_ptr<tools::ToolLoopManager> m_toolLoopManager;

  // These fields are used to detect a selection tool cancelation
  // (deselect command) when the user just click (press and release
  // the mouse button in the "same location" approximately).
  bool m_mouseMoveReceived;
  gfx::Point m_mouseMaxDelta;
  gfx::Point m_mouseDownPos;
  base::tick_t m_mouseDownTime;

  // Stores the last mouse pointer, used to re-use the latest mouse
  // button when onScrollChange() event is received.
  tools::Pointer m_lastPointer;

  // Used to calculate the velocity of the mouse (whch is a sensor
  // to generate dynamic parameters).
  tools::VelocitySensor m_velocity;

  // Used to know if the button was pressed after the DrawingState
  // was initialized. In this way we can cancel the ToolLoop if the
  // Shift press is used to draw a line, but then released without a
  // mouse click.
  bool m_mousePressedReceived;

  // Locks the scroll
  bool m_processScrollChange;

  obs::scoped_connection m_beforeCmdConn;
};

} // namespace app

#endif // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
