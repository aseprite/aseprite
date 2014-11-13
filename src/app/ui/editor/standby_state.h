/* Aseprite
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

#ifndef APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#define APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/editor_state.h"
#include "app/ui/editor/handle_type.h"
#include "gfx/transformation.h"

namespace app {
  class TransformHandles;

  class StandbyState : public EditorState {
  public:
    StandbyState();
    virtual ~StandbyState();
    virtual void onAfterChangeState(Editor* editor) override;
    virtual void onCurrentToolChange(Editor* editor) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    // Returns true as the standby state is the only one which shows
    // the brush-preview.
    virtual bool requireBrushPreview() override { return true; }

    virtual gfx::Transformation getTransformation(Editor* editor);

    void startSelectionTransformation(Editor* editor, const gfx::Point& move);

  protected:
    // Returns true and changes to ScrollingState when "msg" says "the
    // user wants to scroll".
    bool checkForScroll(Editor* editor, ui::MouseMessage* msg);
    bool checkForZoom(Editor* editor, ui::MouseMessage* msg);

    class Decorator : public EditorDecorator {
    public:
      Decorator(StandbyState* standbyState);
      virtual ~Decorator();

      TransformHandles* getTransformHandles(Editor* editor);

      bool onSetCursor(Editor* editor);

      // EditorDecorator overrides
      void preRenderDecorator(EditorPreRender* render) override;
      void postRenderDecorator(EditorPostRender* render) override;
    private:
      TransformHandles* m_transfHandles;
      StandbyState* m_standbyState;
    };

  private:
    void transformSelection(Editor* editor, ui::MouseMessage* msg, HandleType handle);
    void callEyedropper(Editor* editor);

    Decorator* m_decorator;
  };

} // namespace app

#endif  // APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
