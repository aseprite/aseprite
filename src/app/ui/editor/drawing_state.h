// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#define APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"
#include "obs/connection.h"
#include <memory>

namespace app {
  namespace tools {
    class Pointer;
    class ToolLoop;
    class ToolLoopManager;
  }

  class CommandExecutionEvent;

  class DrawingState : public StandbyState {
  public:
    DrawingState(Editor* editor,
                 tools::ToolLoop* loop,
                 const DrawingType type);
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

    void initToolLoop(Editor* editor,
                      const tools::Pointer& pointer);

    // Used to send a movement() to the ToolLoopManager when
    // Shift+brush tool is used to paint a line.
    void sendMovementToToolLoop(const tools::Pointer& pointer);

    void notifyToolLoopModifiersChange(Editor* editor);

  private:
    bool canExecuteCommands();
    void onBeforeCommandExecution(CommandExecutionEvent& cmd);
    void destroyLoopIfCanceled(Editor* editor);
    void destroyLoop(Editor* editor);

    Editor* m_editor;
    DrawingType m_type;

    // The tool-loop.
    std::unique_ptr<tools::ToolLoop> m_toolLoop;

    // Tool-loop manager
    std::unique_ptr<tools::ToolLoopManager> m_toolLoopManager;

    // True if at least we've received a onMouseMove(). It's used to
    // cancel selection tool (deselect) when the user click (press and
    // release the mouse button in the same location).
    bool m_mouseMoveReceived;

    // Stores the last drawing position before we start this
    // DrawingState. It's used to restore the last drawing position in
    // case this stroke is canceled.
    gfx::Point m_lastPoint;

    // Used to know if the button was pressed after the DrawingState
    // was initialized. In this way we can cancel the ToolLoop if the
    // Shift press is used to draw a line, but then released without a
    // mouse click.
    bool m_mousePressedReceived;

    obs::scoped_connection m_beforeCmdConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
