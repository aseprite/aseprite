// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_ZOOMING_STATE_H_INCLUDED
#define APP_UI_EDITOR_ZOOMING_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_state.h"
#include "gfx/point.h"
#include "render/zoom.h"

namespace app {

class ZoomingState : public EditorState {
public:
  ZoomingState();
  virtual bool isTemporalState() const override { return true; }
  virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onUpdateStatusBar(Editor* editor) override;

private:
  gfx::Point m_startPos;
  render::Zoom m_startZoom;
  bool m_moved;
};

} // namespace app

#endif // APP_UI_EDITOR_ZOOMING_STATE_H_INCLUDED
