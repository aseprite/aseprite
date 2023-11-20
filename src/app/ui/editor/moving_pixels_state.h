// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
#pragma once

#include "app/ui/context_bar_observer.h"
#include "app/ui/editor/delayed_mouse_move.h"
#include "app/ui/editor/editor_observer.h"
#include "app/ui/editor/handle_type.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline_observer.h"
#include "obs/connection.h"
#include "ui/timer.h"

namespace doc {
  class Image;
}

namespace app {
  class CommandExecutionEvent;
  class Editor;

  class MovingPixelsState
    : public StandbyState
    , EditorObserver
    , TimelineObserver
    , ContextBarObserver
    , PixelsMovementDelegate
    , DelayedMouseMoveDelegate {
  public:
    MovingPixelsState(Editor* editor,
                      ui::MouseMessage* msg,
                      PixelsMovementPtr pixelsMovement,
                      HandleType handle);
    virtual ~MovingPixelsState();

    bool canHandleFrameChange() const {
      return m_pixelsMovement->canHandleFrameChange();
    }

    void translate(const gfx::PointF& delta);
    void rotate(double angle);
    void flip(doc::algorithm::FlipType flipType);
    void shift(int dx, int dy);

    void updateTransformation(const Transformation& t);

    // EditorState
    virtual void onEnterState(Editor* editor) override;
    virtual void onEditorGotFocus(Editor* editor) override;
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    virtual void onActiveToolChange(Editor* editor, tools::Tool* tool) override;
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

    // TimelineObserver
    virtual void onBeforeRangeChanged(Timeline* timeline) override;

    // ContextBarObserver
    virtual void onDropPixels(ContextBarObserver::DropAction action) override;

    // PixelsMovementDelegate
    virtual void onPivotChange() override;

    virtual Transformation getTransformation(Editor* editor) override;

  private:
    // DelayedMouseMoveDelegate impl
    void onCommitMouseMove(Editor* editor,
                           const gfx::PointF& spritePos) override;

    void onTransparentColorChange();
    void onRenderTimer();

    // ContextObserver
    void onBeforeCommandExecution(CommandExecutionEvent& ev);

    void setTransparentColor(bool opaque, const app::Color& color);
    void dropPixels();

    bool isActiveDocument() const;
    bool isActiveEditor() const;

    void removeAsEditorObserver();
    void removePixelsMovement();

    KeyAction getCurrentKeyAction() const;

    // Helper member to move/translate selection and pixels.
    PixelsMovementPtr m_pixelsMovement;
    DelayedMouseMove m_delayedMouseMove;
    Editor* m_editor;
    bool m_observingEditor;

    // True if the image was discarded (e.g. when a "Cut" command was
    // used to remove the dragged image).
    bool m_discarded;

    // Variable to store the initial key action to ignore it until we
    // re-press the key. This was done mainly to avoid activating the
    // fine control with the Ctrl key when we copy the selection until
    // the user release and press again the Ctrl key.
    KeyAction m_lockedKeyAction = KeyAction::None;

    ui::Timer m_renderTimer;

    obs::connection m_ctxConn;
    obs::connection m_opaqueConn;
    obs::connection m_transparentConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
