// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_PLAY_STATE_H_INCLUDED
#define APP_UI_EDITOR_PLAY_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"
#include "base/time.h"
#include "doc/frame.h"
#include "doc/playback.h"
#include "obs/connection.h"
#include "ui/timer.h"

namespace doc {
class Tag;
}

namespace app {

class CommandExecutionEvent;

class PlayState : public StateWithWheelBehavior {
public:
  PlayState(const bool playOnce, const bool playAll, const bool playSubtags);

  doc::Tag* playingTag() const;

  void onEnterState(Editor* editor) override;
  LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
  void onBeforePopState(Editor* editor) override;
  bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
  bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
  void onRemoveTag(Editor* editor, doc::Tag* tag) override;

private:
  void onPlaybackTick();

  // ContextObserver
  void onBeforeCommandExecution(CommandExecutionEvent& ev);

  double getNextFrameTime();

  Editor* m_editor;
  doc::Playback m_playback;
  bool m_playOnce;
  bool m_playAll;
  bool m_playSubtags;
  bool m_toScroll;
  ui::Timer m_playTimer;

  // Number of milliseconds to go to the next frame if m_playTimer
  // is activated.
  double m_nextFrameTime;
  base::tick_t m_curFrameTick;

  doc::frame_t m_refFrame;
  doc::Tag* m_tag;

  obs::scoped_connection m_ctxConn;
};

} // namespace app

#endif
