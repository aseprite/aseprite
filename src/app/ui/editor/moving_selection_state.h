// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_SELECTION_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_SELECTION_STATE_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/ui/editor/standby_state.h"
#include "obs/connection.h"

namespace app {
  class MovingSelectionState : public StandbyState {
  public:
    MovingSelectionState(Editor* editor, ui::MouseMessage* msg);

    // EditorState
    virtual void onEnterState(Editor* editor) override;
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;
    virtual bool requireBrushPreview() override { return false; }

  private:
    // ContextObserver
    void onBeforeCommandExecution(CommandExecutionEvent& ev);

    Editor* m_editor;
    gfx::Point m_cursorStart;
    gfx::Point m_selOrigin;
    gfx::Point m_delta;
    obs::scoped_connection m_ctxConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
