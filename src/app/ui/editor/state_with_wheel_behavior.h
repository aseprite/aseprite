// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_STATE_WITH_WHEEL_BEHAVIOR_H_INCLUDED
#define APP_UI_STATE_WITH_WHEEL_BEHAVIOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/tools/ink_type.h"
#include "app/ui/editor/editor_state.h"
#include "app/ui/key.h"
#include "doc/frame.h"
#include "doc/layer.h"
#include "doc/layer_list.h"

namespace render {
  class Zoom;
}

namespace tools {
  class Tool;
}

namespace app {

  class StateWithWheelBehavior : public EditorState {
  public:
    StateWithWheelBehavior();

    bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override;
    bool onTouchMagnify(Editor* editor, ui::TouchMessage* msg) override;
    bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;

  protected:
    void processWheelAction(Editor* editor,
                            WheelAction wheelAction,
                            const gfx::Point& position,
                            gfx::Point delta,
                            double dz,
                            bool scrollBigSteps,
                            bool preciseWheel);
    const doc::LayerList& browsableLayers(Editor* editor) const;

    virtual Color initialFgColor() const;
    virtual Color initialBgColor() const;
    virtual int initialFgTileIndex() const;
    virtual int initialBgTileIndex() const;
    virtual int initialBrushSize();
    virtual int initialBrushAngle();
    virtual gfx::Point initialScroll(Editor* editor) const;
    virtual render::Zoom initialZoom(Editor* editor) const;
    virtual doc::frame_t initialFrame(Editor* editor) const;
    virtual doc::layer_t initialLayer(Editor* editor) const;
    virtual tools::InkType initialInkType(Editor* editor) const;
    virtual int initialInkOpacity(Editor* editor) const;
    virtual int initialCelOpacity(Editor* editor) const;
    virtual int initialLayerOpacity(Editor* editor) const;
    virtual tools::Tool* initialTool() const;
    virtual void changeFgColor(Color c);

  private:
    void setZoom(Editor* editor, const render::Zoom& zoom, const gfx::Point& mousePos);
    tools::Tool* getActiveTool() const;
    void disableQuickTool() const;

    mutable doc::LayerList m_browsableLayers;
    tools::Tool* m_groupTool;
  };

} // namespace app

#endif
