// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_WRITING_TEXT_STATE_H_INCLUDED
#define APP_UI_EDITOR_WRITING_TEXT_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"

#include <memory>

namespace app {
  class CommandExecutionEvent;

  class WritingTextState : public StandbyState {
  public:
    WritingTextState(Editor* editor,
                     const gfx::Rect& bounds);
    ~WritingTextState();

    LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    void onEnterState(Editor* editor) override;
    void onBeforePopState(Editor* editor) override;
    void onEditorGotFocus(Editor* editor) override;
    void onEditorResize(Editor* editor) override;
    bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;

  private:
    gfx::Rect calcEntryBounds();
    void onBeforeCommandExecution(CommandExecutionEvent& ev);
    void onFontChange();
    void cancel();
    void drop();

    void switchCaretVisiblity();

    class TextEditor;

    Editor* m_editor;
    gfx::Rect m_bounds;
    std::unique_ptr<TextEditor> m_entry;

    // True if the text was discarded.
    bool m_discarded = false;

    obs::scoped_connection m_beforeCmdConn;
    obs::scoped_connection m_fontChangeConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
