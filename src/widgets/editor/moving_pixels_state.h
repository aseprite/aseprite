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

#ifndef WIDGETS_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
#define WIDGETS_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED

#include "base/compiler_specific.h"
#include "context_observer.h"
#include "widgets/editor/handle_type.h"
#include "widgets/editor/standby_state.h"
#include "widgets/status_bar.h"

class Editor;
class Image;
class PixelsMovement;

class MovingPixelsState : public StandbyState, StatusBarObserver, ContextObserver
{
public:
  MovingPixelsState(Editor* editor, ui::MouseMessage* msg, PixelsMovement* pixelsMovement, HandleType handle);
  virtual ~MovingPixelsState();

  // EditorState
  virtual BeforeChangeAction onBeforeChangeState(Editor* editor, EditorState* newState) OVERRIDE;
  virtual void onCurrentToolChange(Editor* editor) OVERRIDE;
  virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) OVERRIDE;
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) OVERRIDE;
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) OVERRIDE;
  virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) OVERRIDE;
  virtual bool onSetCursor(Editor* editor) OVERRIDE;
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) OVERRIDE;
  virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) OVERRIDE;
  virtual bool onUpdateStatusBar(Editor* editor) OVERRIDE;

  // ContextObserver
  virtual void onCommandBeforeExecution(Context* context) OVERRIDE;

  virtual gfx::Transformation getTransformation(Editor* editor) OVERRIDE;

protected:
  // StatusBarObserver interface
  virtual void dispose() OVERRIDE;
  virtual void onChangeTransparentColor(const app::Color& color) OVERRIDE;

private:
  void setTransparentColor(const app::Color& color);
  void dropPixels(Editor* editor);

  // Helper member to move/translate selection and pixels.
  PixelsMovement* m_pixelsMovement;
  Editor* m_currentEditor;

  // True if the image was discarded (e.g. when a "Cut" command was
  // used to remove the dragged image).
  bool m_discarded;
};

#endif  // WIDGETS_EDITOR_MOVING_PIXELS_STATE_H_INCLUDED
