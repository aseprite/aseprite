/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef WIDGETS_EDITOR_STANDBY_STATE_H_INCLUDED
#define WIDGETS_EDITOR_STANDBY_STATE_H_INCLUDED

#include "base/compiler_specific.h"
#include "gfx/transformation.h"
#include "widgets/editor/editor_decorator.h"
#include "widgets/editor/editor_state.h"
#include "widgets/editor/handle_type.h"

class TransformHandles;

class StandbyState : public EditorState
{
public:
  StandbyState();
  virtual ~StandbyState();
  virtual void onAfterChangeState(Editor* editor) OVERRIDE;
  virtual void onCurrentToolChange(Editor* editor) OVERRIDE;
  virtual bool onMouseDown(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onMouseUp(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onMouseMove(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onMouseWheel(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onSetCursor(Editor* editor) OVERRIDE;
  virtual bool onKeyDown(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onKeyUp(Editor* editor, ui::Message* msg) OVERRIDE;
  virtual bool onUpdateStatusBar(Editor* editor) OVERRIDE;

  // Returns true as the standby state is the only one which shows the
  // pen-preview.
  virtual bool requirePenPreview() OVERRIDE { return true; }

  virtual gfx::Transformation getTransformation(Editor* editor);

protected:
  // Returns true and changes to ScrollingState when "msg" says "the
  // user wants to scroll".
  bool checkForScroll(Editor* editor, ui::Message* msg);

  class Decorator : public EditorDecorator
  {
  public:
    Decorator(StandbyState* standbyState);
    virtual ~Decorator();

    TransformHandles* getTransformHandles(Editor* editor);

    bool onSetCursor(Editor* editor);

    // EditorDecorator overrides
    void preRenderDecorator(EditorPreRender* render) OVERRIDE;
    void postRenderDecorator(EditorPostRender* render) OVERRIDE;
  private:
    TransformHandles* m_transfHandles;
    StandbyState* m_standbyState;
  };

private:
  void transformSelection(Editor* editor, ui::Message* msg, HandleType handle);

  Decorator* m_decorator;
};

#endif  // WIDGETS_EDITOR_STANDBY_STATE_H_INCLUDED
