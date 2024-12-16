// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_DELAYED_MOUSE_MOVE_STATE_H_INCLUDED
#define APP_UI_EDITOR_DELAYED_MOUSE_MOVE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"
#include "ui/timer.h"

#include <limits>

namespace ui {
class MouseMessage;
}

namespace app {
class Editor;

class DelayedMouseMoveDelegate {
public:
  virtual ~DelayedMouseMoveDelegate() {}
  virtual void onCommitMouseMove(Editor* editor, const gfx::PointF& spritePos) = 0;
};

// Helper class to group several onMouseMove() calls into one
// onCommitMouseMove(). Useful in Linux as mouse movement messages
// are queued a high speed (so we can group several mouse messages
// for tools that use only the last position and have a heavy
// calculation per mouse position, e.g. pixel transformations,
// drawing with Line, Rectangle, etc.).
class DelayedMouseMove {
public:
  // The "interval" is given in milliseconds, and can be zero if we
  // want to disable the delay between onMouseMove() -> onCommitMouseMove()
  DelayedMouseMove(DelayedMouseMoveDelegate* delegate, Editor* editor, const int interval);

  // In case the event wasn't started with onMouseDown() we can
  // initialize the sprite position directly (e.g. starting a line
  // from last painted point with Shift+click with Pencil tool).
  void initSpritePos(const gfx::PointF& pos);

  void onMouseDown(const ui::MouseMessage* msg);
  bool onMouseMove(const ui::MouseMessage* msg);
  void onMouseUp(const ui::MouseMessage* msg);

  // Forces stopping the timer so we don't receive a Tick event in
  // case that we've just removed/delete some structure that is
  // needed in onCommitMouseMove().
  void stopTimer();

  const gfx::PointF& spritePos() const;

private:
  void commitMouseMove();
  bool updateSpritePos(const ui::MouseMessage* msg);

  DelayedMouseMoveDelegate* m_delegate;
  Editor* m_editor;
  ui::Timer m_timer;

  // Position of the mouse in the canvas to avoid redrawing when the
  // mouse position changes (only we redraw when the canvas position
  // changes).
  gfx::PointF m_spritePos;
};

} // namespace app

#endif // APP_UI_EDITOR_DELAYED_MOUSE_MOVE_STATE_H_INCLUDED
