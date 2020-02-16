// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#define APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
#pragma once

#include "app/transformation.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/handle_type.h"
#include "app/ui/editor/state_with_wheel_behavior.h"
#include "doc/algorithm/flip_type.h"
#include "obs/connection.h"

namespace app {
  namespace tools {
    class Ink;
    class Pointer;
    class Command;
  }

  class DrawingState;
  class TransformHandles;

  class StandbyState : public StateWithWheelBehavior {
  public:
    enum class DrawingType { Regular, LineFreehand, SelectTiles };

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

    virtual Transformation getTransformation(Editor* editor);

    void startSelectionTransformation(Editor* editor, const gfx::Point& move, double angle);

    void startFlipTransformation(Editor* editor, doc::algorithm::FlipType flipType);

  protected:
    void callEyedropper(Editor* editor, const ui::MouseMessage* msg);
    bool checkStartDrawingStraightLine(Editor* editor, const ui::MouseMessage* msg);
    virtual bool canCheckStartDrawingStraightLine() { return true; }

    class Decorator : public EditorDecorator {
    public:
      struct Handle {
        int align;
        gfx::Rect bounds;
        Handle(int align, const gfx::Rect& bounds)
          : align(align), bounds(bounds) { }
      };
      typedef std::vector<Handle> Handles;

      Decorator(StandbyState* standbyState);
      virtual ~Decorator();

      TransformHandles* getTransformHandles(Editor* editor);
      bool getSymmetryHandles(Editor* editor, Handles& handles);

      bool onSetCursor(tools::Ink* ink, Editor* editor, const gfx::Point& mouseScreenPos);

      // EditorDecorator overrides
      void postRenderDecorator(EditorPostRender* render) override;
      void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override;

    private:
      TransformHandles* m_transfHandles;
      StandbyState* m_standbyState;
    };

  private:
    DrawingState* startDrawingState(Editor* editor,
                                    const DrawingType drawingType,
                                    const tools::Pointer& pointer);
    void transformSelection(Editor* editor, ui::MouseMessage* msg, HandleType handle);
    void onPivotChange(Editor* editor);
    gfx::Rect resizeCelBounds(Editor* editor) const;
    bool overSelectionEdges(Editor* editor, const gfx::Point& mouseScreenPos) const;
    void selectTile(Editor* editor);

    Decorator* m_decorator;
    obs::scoped_connection m_pivotVisConn;
    obs::scoped_connection m_pivotPosConn;
    bool m_transformSelectionHandlesAreVisible;
  };

} // namespace app

#endif  // APP_UI_EDITOR_STANDBY_STATE_H_INCLUDED
