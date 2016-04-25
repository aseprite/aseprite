// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#define APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/handle_type.h"
#include "app/ui/editor/state_with_wheel_behavior.h"
#include "base/connection.h"
#include "gfx/transformation.h"

namespace app {
  namespace tools {
    class Ink;
  }

  class TransformHandles;

  class StandbyState : public StateWithWheelBehavior {
  public:
    StandbyState();
    virtual ~StandbyState();
    virtual void onEnterState(Editor* editor) override;
    virtual void onActiveToolChange(Editor* editor, tools::Tool* tool) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onDoubleClick(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    // Returns true as the standby state is the only one which shows
    // the brush-preview.
    virtual bool requireBrushPreview() override { return true; }

    virtual gfx::Transformation getTransformation(Editor* editor);

    void startSelectionTransformation(Editor* editor, const gfx::Point& move, double angle);

  protected:
    // Returns true and changes to ScrollingState when "msg" says "the
    // user wants to scroll".
    bool checkForScroll(Editor* editor, ui::MouseMessage* msg);
    bool checkForZoom(Editor* editor, ui::MouseMessage* msg);
    void callEyedropper(Editor* editor);

    class Decorator : public EditorDecorator {
    public:
      Decorator(StandbyState* standbyState);
      virtual ~Decorator();

      TransformHandles* getTransformHandles(Editor* editor);
      bool getSymmetryHandles(Editor* editor, gfx::Rect& box1, gfx::Rect& box2);

      bool onSetCursor(tools::Ink* ink, Editor* editor, const gfx::Point& mouseScreenPos);

      // EditorDecorator overrides
      void preRenderDecorator(EditorPreRender* render) override;
      void postRenderDecorator(EditorPostRender* render) override;
      void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override;

    private:
      TransformHandles* m_transfHandles;
      StandbyState* m_standbyState;
    };

  private:
    void transformSelection(Editor* editor, ui::MouseMessage* msg, HandleType handle);
    void onPivotChange(Editor* editor);

    Decorator* m_decorator;
    base::ScopedConnection m_pivotVisConn;
    base::ScopedConnection m_pivotPosConn;
  };

} // namespace app

#endif  // APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
