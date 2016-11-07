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
#include "app/ui/editor/handle_type.h"
#include "doc/cel_list.h"

#include <vector>

namespace doc {
  class Cel;
}

namespace app {
  class Editor;

  class MovingCelState : public StandbyState {
  public:
    MovingCelState(Editor* editor,
                   ui::MouseMessage* msg,
                   const HandleType handle);

    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    virtual bool requireBrushPreview() override { return false; }

  private:
    ContextReader m_reader;
    Cel* m_cel;
    CelList m_celList;
    std::vector<gfx::RectF> m_celStarts;
    gfx::PointF m_cursorStart;
    gfx::PointF m_celOffset;
    gfx::SizeF m_celMainSize;
    gfx::SizeF m_celScale;
    bool m_canceled;
    bool m_maskVisible;
    bool m_hasReference;
    bool m_scaled;
    HandleType m_handle;
  };

} // namespace app

#endif  // APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
