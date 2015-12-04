// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
#pragma once

#include "app/ui/context_bar_observer.h"
#include "app/ui/editor/editor_observer.h"
#include "app/ui/editor/handle_type.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/status_bar.h"
#include "base/connection.h"

namespace doc {
  class Image;
}

namespace app {
  class CommandExecutionEvent;
  class Editor;

  class MovingPixelsState
    : public StandbyState
    , EditorObserver
    , ContextBarObserver {
  public:
    MovingPixelsState(Editor* editor, ui::MouseMessage* msg, PixelsMovementPtr pixelsMovement, HandleType handle);
    virtual ~MovingPixelsState();

    void translate(const gfx::Point& delta);
    void rotate(double angle);

    // EditorState
    virtual void onEnterState(Editor* editor) override;
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    virtual void onCurrentToolChange(Editor* editor) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;
    virtual bool acceptQuickTool(tools::Tool* tool) override;
    virtual bool requireBrushPreview() override { return false; }

    // EditorObserver
    virtual void onDestroyEditor(Editor* editor) override;
    virtual void onBeforeFrameChanged(Editor* editor) override;
    virtual void onBeforeLayerChanged(Editor* editor) override;

    // ContextBarObserver
    virtual void onDropPixels(ContextBarObserver::DropAction action) override;

    virtual gfx::Transformation getTransformation(Editor* editor) override;

  private:
    void onTransparentColorChange();

    // ContextObserver
    void onBeforeCommandExecution(CommandExecutionEvent& ev);

    void setTransparentColor(bool opaque, const app::Color& color);
    void dropPixels();

    bool isActiveDocument() const;
    bool isActiveEditor() const;

    void removeAsEditorObserver();

    // Helper member to move/translate selection and pixels.
    PixelsMovementPtr m_pixelsMovement;
    Editor* m_editor;
    bool m_observingEditor;

    // True if the image was discarded (e.g. when a "Cut" command was
    // used to remove the dragged image).
    bool m_discarded;

    base::ScopedConnection m_ctxConn;
    base::ScopedConnection m_opaqueConn;
    base::ScopedConnection m_transparentConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
