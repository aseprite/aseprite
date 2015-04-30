// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_PLAY_STATE_H_INCLUDED
#define APP_UI_EDITOR_PLAY_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"
#include "doc/frame.h"
#include "ui/timer.h"

namespace app {

  class Command;

  class PlayState : public StateWithWheelBehavior {
  public:
    PlayState();

    void onAfterChangeState(Editor* editor) override;
    BeforeChangeAction onBeforeChangeState(Editor* editor, EditorState* newState) override;
    bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;

  private:
    void onPlaybackTick();

    // ContextObserver
    void onBeforeCommandExecution(Command* command);

    Editor* m_editor;
    ui::Timer m_playTimer;

    // Number of milliseconds to go to the next frame if m_playTimer
    // is activated.
    int m_nextFrameTime;
    int m_curFrameTick;

    bool m_pingPongForward;
    doc::frame_t m_refFrame;

    Connection m_ctxConn;
  };

} // namespace app

#endif
