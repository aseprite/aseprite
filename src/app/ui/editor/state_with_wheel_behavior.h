// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_STATE_WITH_WHEEL_BEHAVIOR_H_INCLUDED
#define APP_UI_STATE_WITH_WHEEL_BEHAVIOR_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_state.h"

namespace render {
  class Zoom;
}

namespace tools {
  class Tool;
}

namespace app {

  class StateWithWheelBehavior : public EditorState {
  public:
    virtual bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onTouchMagnify(Editor* editor, ui::TouchMessage* msg) override;
  private:
    void setZoom(Editor* editor, const render::Zoom& zoom, const gfx::Point& mousePos);
    tools::Tool* getActiveTool();
    void disableQuickTool();
  };

} // namespace app

#endif
