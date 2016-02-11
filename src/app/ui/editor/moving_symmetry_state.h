// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_MOVING_SYMMETRY_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_SYMMETRY_STATE_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/ui/editor/standby_state.h"

namespace app {
  class Editor;

  class MovingSymmetryState : public StandbyState {
  public:
    MovingSymmetryState(Editor* editor, ui::MouseMessage* msg,
                        app::gen::SymmetryMode mode,
                        Option<int>& symmetryAxis);
    virtual ~MovingSymmetryState();

    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

    virtual bool requireBrushPreview() override { return false; }

  private:
    app::gen::SymmetryMode m_symmetryMode;
    Option<int>& m_symmetryAxis;
    int m_symmetryAxisStart;
    gfx::Point m_mouseStart;
  };

} // namespace app

#endif
