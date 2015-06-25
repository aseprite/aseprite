// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_SCROLLING_STATE_H_INCLUDED
#define APP_UI_EDITOR_SCROLLING_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_state.h"
#include "gfx/point.h"

namespace app {

  class ScrollingState : public EditorState {
  public:
    ScrollingState();
    virtual ~ScrollingState();
    virtual bool isTemporalState() const override { return true; }
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onUpdateStatusBar(Editor* editor) override;

  private:
    gfx::Point m_oldPos;
  };

} // namespace app

#endif  // APP_UI_EDITOR_SCROLLING_STATE_H_INCLUDED
