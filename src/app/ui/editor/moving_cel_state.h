// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"

#include "app/context_access.h"
#include "doc/cel_list.h"

#include <vector>

namespace doc {
  class Cel;
}

namespace app {
  class Editor;

  class MovingCelState : public StandbyState {
  public:
    MovingCelState(Editor* editor, ui::MouseMessage* msg);

    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    virtual bool requireBrushPreview() override { return false; }

  private:
    ContextReader m_reader;
    CelList m_celList;
    std::vector<gfx::RectF> m_celStarts;
    gfx::PointF m_cursorStart;
    gfx::PointF m_celOffset;
    bool m_canceled;
    bool m_maskVisible;
    bool m_hasReference;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
