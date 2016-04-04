// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#define APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"

namespace app {
  namespace tools {
    class ToolLoop;
    class ToolLoopManager;
  }

  class DrawingState : public StandbyState {
  public:
    DrawingState(tools::ToolLoop* loop);
    virtual ~DrawingState();
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;
    virtual void onExposeSpritePixels(const gfx::Region& rgn) override;

    // Drawing state doesn't require the brush-preview because we are
    // already drawing (viewing the real trace).
    virtual bool requireBrushPreview() override { return false; }

    void initToolLoop(Editor* editor, ui::MouseMessage* msg);
    void notifyToolLoopModifiersChange(Editor* editor);

  private:
    void destroyLoopIfCanceled(Editor* editor);
    void destroyLoop(Editor* editor);

    // The tool-loop.
    tools::ToolLoop* m_toolLoop;

    // Tool-loop manager
    tools::ToolLoopManager* m_toolLoopManager;

    // True if at least we've received a onMouseMove(). It's used to
    // cancel selection tool (deselect) when the user click (press and
    // release the mouse button in the same location).
    bool m_mouseMoveReceived;

    // Stores the last drawing position before we start this
    // DrawingState. It's used to restore the last drawing position in
    // case this stroke is canceled.
    gfx::Point m_lastPoint;
  };

} // namespace app

#endif  // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
