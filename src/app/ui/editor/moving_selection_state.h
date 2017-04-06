// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_SELECTION_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_SELECTION_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"

namespace app {
  class MovingSelectionState : public StandbyState {
  public:
    MovingSelectionState(Editor* editor, ui::MouseMessage* msg);
    virtual ~MovingSelectionState();

    // EditorState
    virtual void onEnterState(Editor* editor) override;
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;
    virtual bool requireBrushPreview() override { return false; }

  private:
    gfx::Point m_cursorStart;
    gfx::Point m_selOrigin;
    gfx::Point m_delta;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
