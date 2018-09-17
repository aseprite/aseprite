// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_EDITOR_STATE_H_INCLUDED
#define APP_UI_EDITOR_EDITOR_STATE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/shared_ptr.h"
#include "gfx/point.h"

namespace gfx {
  class Region;
}

namespace ui {
  class KeyMessage;
  class MouseMessage;
  class TouchMessage;
}

namespace doc {
  class FrameTag;
}

namespace app {
  class Editor;
  class EditorDecorator;

  namespace tools {
    class Ink;
    class Tool;
  }

  // Represents one state of the sprite's editor (Editor class).  This
  // is a base class, a dummy state that ignores all events from the
  // Editor. Subclasses overrides these methods to customize the
  // behavior of the Editor to do different tasks (e.g. scrolling,
  // drawing in the active sprite, etc.).
  class EditorState {
  public:
    enum LeaveAction {
      DiscardState,
      KeepState
    };

    EditorState() { }
    virtual ~EditorState() { }

    // Returns true if this state is a "temporal" state. It means that
    // this state doesn't go to other state than the previous one.
    virtual bool isTemporalState() const { return false; }

    // Called just before this state is replaced by a new state in the
    // Editor::setState() method.  Returns true if this state should be
    // kept in the EditorStatesHistory.
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) {
      return KeepState;
    }

    // Called when this instance is set as the new Editor's state when
    // Editor::setState() method is used.
    virtual void onEnterState(Editor* editor) { }

    // Called just before the state will be removed from the
    // EditorStatesHistory.  This event is useful to remove the
    // decorator from the editor.
    virtual void onBeforePopState(Editor* editor) { }

    // Called when the current tool in the tool bar changes. It is
    // useful for states which depends on the selected current tool (as
    // MovingPixelsState which drops the pixels in case the user selects
    // other drawing tool).
    virtual void onActiveToolChange(Editor* editor, tools::Tool* tool) { }

    // Called when the user presses a mouse button over the editor.
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) { return false; }

    // Called when the user releases a mouse button.
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) { return false; }

    // Called when the user moves the mouse over the editor.
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) { return false; }

    // Called when the user moves the mouse wheel over the editor.
    virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) { return false; }

    // Called when the user wants to zoom in/out using a pinch gesture in the trackpad.
    virtual bool onTouchMagnify(Editor* editor, ui::TouchMessage* msg) { return false; }

    // Called when the user moves the mouse wheel over the editor.
    virtual bool onDoubleClick(Editor* editor, ui::MouseMessage* msg) { return false; }

    // Called each time the mouse changes its position so we can set an
    // appropiated cursor depending on the new coordinates of the mouse
    // pointer.
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) { return false; }

    // Called when a key is pressed over the current editor.
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) { return false; }

    // Called when a key is released.
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) { return false; }

    // Called when status bar needs to be updated.
    virtual bool onUpdateStatusBar(Editor* editor) { return false; }

    // When a part of the sprite will be exposed.
    virtual void onExposeSpritePixels(const gfx::Region& rgn) { }

    // Returns true if the this state requires the brush-preview as
    // drawing cursor.
    virtual bool requireBrushPreview() { return false; }

    // Returns true if this state accept the given quicktool.
    virtual bool acceptQuickTool(tools::Tool* tool) { return true; }

    // Custom ink in this state.
    virtual tools::Ink* getStateInk() { return nullptr; }

    // Called when a tag is deleted.
    virtual void onRemoveFrameTag(Editor* editor, doc::FrameTag* tag) { }

  private:
    DISABLE_COPYING(EditorState);
  };

  typedef base::SharedPtr<EditorState> EditorStatePtr;

} // namespace app

#endif  // APP_UI_EDITOR_EDITOR_STATE_H_INCLUDED
