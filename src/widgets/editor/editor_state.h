/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef WIDGETS_EDITOR_EDITOR_STATE_H_INCLUDED
#define WIDGETS_EDITOR_EDITOR_STATE_H_INCLUDED

#include "base/disable_copying.h"
#include "base/shared_ptr.h"

class Editor;
class EditorDecorator;
namespace ui {
  class MouseMessage;
  class KeyMessage;
}

// Represents one state of the sprite's editor (Editor class).  This
// is a base class, a dummy state that ignores all events from the
// Editor. Subclasses overrides these methods to customize the
// behavior of the Editor to do different tasks (e.g. scrolling,
// drawing in the active sprite, etc.).
class EditorState
{
public:
  enum BeforeChangeAction {
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
  virtual BeforeChangeAction onBeforeChangeState(Editor* editor, EditorState* newState) {
    return KeepState;
  }

  // Called when this instance is set as the new Editor's state when
  // Editor::setState() method is used.
  virtual void onAfterChangeState(Editor* editor) { }

  // Called just before the state will be removed from the
  // EditorStatesHistory.  This event is useful to remove the
  // decorator from the editor.
  virtual void onBeforePopState(Editor* editor) { }

  // Called when the current tool in the tool bar changes. It is
  // useful for states which depends on the selected current tool (as
  // MovingPixelsState which drops the pixels in case the user selects
  // other drawing tool).
  virtual void onCurrentToolChange(Editor* editor) { }

  // Called when the user presses a mouse button over the editor.
  virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) { return false; }

  // Called when the user releases a mouse button.
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) { return false; }

  // Called when the user moves the mouse over the editor.
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) { return false; }

  // Called when the user moves the mouse wheel over the editor.
  virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) { return false; }

  // Called each time the mouse changes its position so we can set an
  // appropiated cursor depending on the new coordinates of the mouse
  // pointer.
  virtual bool onSetCursor(Editor* editor) { return false; }

  // Called when a key is pressed over the current editor.
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) { return false; }

  // Called when a key is released.
  virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) { return false; }

  // Called when a key is released.
  virtual bool onUpdateStatusBar(Editor* editor) { return false; }

  // Returns true if the this state requires the pen-preview as
  // drawing cursor.
  virtual bool requirePenPreview() { return false; }

private:
  DISABLE_COPYING(EditorState);
};

typedef SharedPtr<EditorState> EditorStatePtr;

#endif  // WIDGETS_EDITOR_EDITOR_STATE_H_INCLUDED
