// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
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

namespace app {

  namespace tools {
    class Tool;
    class ToolGroup;
  }

  class StateWithWheelBehavior : public EditorState {
  public:
    enum class ScrollBigSteps { Off, On };
    enum class PreciseWheel { Off, On };

    // Indicates that the message comes from a real mouse wheel (which
    // might have special handling inverting the direction of the
    // wheel action depending on the operating system, etc.)
    enum class FromMouseWheel { Off, On };

    StateWithWheelBehavior();

    bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override;
    bool onTouchMagnify(Editor* editor, ui::TouchMessage* msg) override;
    bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    void onBeforeRemoveLayer(Editor* editor) override;

  protected:
    void processWheelAction(Editor* editor,
                            WheelAction wheelAction,
                            const gfx::Point& position,
                            gfx::Point delta,
                            double dz,
                            const ScrollBigSteps scrollBigSteps,
                            const PreciseWheel preciseWheel,
                            const FromMouseWheel fromMouseWheel);
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
    virtual void disableQuickTool() const;

    virtual tools::Tool* getInitialToolInActiveGroup();
    virtual void onToolChange(tools::Tool* tool);
    virtual void onToolGroupChange(Editor* editor,
                                   tools::ToolGroup* group);

    tools::Tool* getActiveTool() const;

  private:
    void setZoom(Editor* editor, const render::Zoom& zoom, const gfx::Point& mousePos);

    mutable doc::LayerList m_browsableLayers;
    tools::Tool* m_tool = nullptr;
  };

} // namespace app

#endif
