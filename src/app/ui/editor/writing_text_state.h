// Aseprite
// Copyright (c) 2022-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_WRITING_TEXT_STATE_H_INCLUDED
#define APP_UI_EDITOR_WRITING_TEXT_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/delayed_mouse_move.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/font_entry.h"

#include <memory>

namespace app {
class CommandExecutionEvent;
class FontInfo;

class WritingTextState : public StandbyState,
                         DelayedMouseMoveDelegate {
public:
  WritingTextState(Editor* editor, const gfx::Rect& bounds);
  ~WritingTextState();

  LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
  void onEnterState(Editor* editor) override;
  void onBeforePopState(Editor* editor) override;
  void onEditorGotFocus(Editor* editor) override;
  void onEditorResize(Editor* editor) override;
  bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
  bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;

private:
  enum class Hit {
    Normal,
    Edges,
  };

  // DelayedMouseMoveDelegate impl
  void onCommitMouseMove(Editor* editor, const gfx::PointF& spritePos) override;

  gfx::Rect calcEntryBounds();
  void onBeforeCommandExecution(CommandExecutionEvent& ev);
  void onFontChange(const FontInfo& fontInfo, FontEntry::From fromField);
  void cancel();
  void drop();

  void switchCaretVisiblity();

  Hit calcHit(Editor* editor, const gfx::Point& mouseScreenPos);

  class TextEditor;

  DelayedMouseMove m_delayedMouseMove;
  Editor* m_editor;
  gfx::RectF m_bounds;
  std::unique_ptr<TextEditor> m_entry;

  // True if the text was discarded.
  bool m_discarded = false;

  // To move text entry bounds when we drag the mouse from the edges
  // of the TextEditor.
  Hit m_hit = Hit::Normal;
  bool m_mouseMoveReceived = false;
  bool m_movingBounds = false;
  gfx::PointF m_cursorStart;
  gfx::PointF m_boundsOrigin;

  obs::scoped_connection m_beforeCmdConn;
  obs::scoped_connection m_fontChangeConn;
};

} // namespace app

#endif // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
