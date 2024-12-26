// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_NAVIGATE_STATE_H_INCLUDED
#define APP_UI_EDITOR_NAVIGATE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"

namespace app {

class NavigateState : public StateWithWheelBehavior {
public:
  NavigateState();
  virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;

protected:
  void disableQuickTool() const override;
};

} // namespace app

#endif
