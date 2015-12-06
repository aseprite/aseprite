// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/play_state.h"

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/loop_tag.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/scrolling_state.h"
#include "app/ui_context.h"
#include "doc/handle_anidir.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"

namespace app {

using namespace ui;

PlayState::PlayState()
  : m_editor(nullptr)
  , m_toScroll(false)
  , m_playTimer(10)
  , m_nextFrameTime(-1)
  , m_pingPongForward(true)
{
  m_playTimer.Tick.connect(&PlayState::onPlaybackTick, this);

  // Hook BeforeCommandExecution signal so we know if the user wants
  // to execute other command, so we can stop the animation.
  m_ctxConn = UIContext::instance()->BeforeCommandExecution.connect(
    &PlayState::onBeforeCommandExecution, this);
}

void PlayState::onEnterState(Editor* editor)
{
  StateWithWheelBehavior::onEnterState(editor);

  if (!m_editor) {
    m_editor = editor;
    m_refFrame = editor->frame();
  }

  m_toScroll = false;
  m_nextFrameTime = getNextFrameTime();
  m_curFrameTick = ui::clock();
  m_pingPongForward = true;

  // Maybe we came from ScrollingState and the timer is already
  // running.
  if (!m_playTimer.isRunning())
    m_playTimer.start();
}

EditorState::LeaveAction PlayState::onLeaveState(Editor* editor, EditorState* newState)
{
  if (!m_toScroll) {
    if (Preferences::instance().general.rewindOnStop())
      m_editor->setFrame(m_refFrame);

    // We don't stop the timer if we are going to the ScrollingState
    // (we keep playing the animation).
    m_playTimer.stop();
  }
  return KeepState;
}

bool PlayState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return true;

  // When an editor is clicked the current view is changed.
  UIContext* context = UIContext::instance();
  context->setActiveView(editor->getDocumentView());

  // A click with right-button stops the animation
  if (msg->buttons() == kButtonRight) {
    editor->stop();
    return true;
  }

  // Set this flag to indicate that we are going to ScrollingState for
  // some time, so we don't change the current frame.
  m_toScroll = true;

  // Start scroll loop
  EditorStatePtr newState(new ScrollingState());
  editor->setState(newState);
  newState->onMouseDown(editor, msg);
  return true;
}

bool PlayState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->releaseMouse();
  return true;
}

bool PlayState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  editor->showBrushPreview(msg->position());
  editor->updateStatusBar();
  return true;
}

bool PlayState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool PlayState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

void PlayState::onPlaybackTick()
{
  if (m_nextFrameTime < 0)
    return;

  m_nextFrameTime -= (ui::clock() - m_curFrameTick);

  doc::Sprite* sprite = m_editor->sprite();
  doc::FrameTag* tag = get_animation_tag(sprite, m_refFrame);

  while (m_nextFrameTime <= 0) {
    doc::frame_t frame = calculate_next_frame(
      sprite, m_editor->frame(), frame_t(1), tag,
      m_pingPongForward);

    m_editor->setFrame(frame);
    m_nextFrameTime += getNextFrameTime();
    m_editor->invalidate();
  }

  m_curFrameTick = ui::clock();
}

// Before executing any command, we stop the animation
void PlayState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  // This check just in case we stay connected to context signals when
  // the editor is already deleted.
  ASSERT(m_editor);
  ASSERT(m_editor->manager() == ui::Manager::getDefault());

  // If the command is for other editor, we don't stop the animation.
  if (!m_editor->isActive())
    return;

  // If we're executing PlayAnimation command, it means that the
  // user wants to stop the animation. We cannot stop the animation
  // here, because if it's stopped, PlayAnimation will re-play it
  // (so it would be impossible to stop the animation using
  // PlayAnimation command/Enter key).
  //
  // There are other commands that just doesn't stop the animation
  // (zoom, scroll, etc.)
  if (ev.command()->id() == CommandId::PlayAnimation ||
      ev.command()->id() == CommandId::Zoom ||
      ev.command()->id() == CommandId::Scroll) {
    return;
  }

  m_editor->stop();
}

double PlayState::getNextFrameTime()
{
  return
    m_editor->sprite()->frameDuration(m_editor->frame())
    / m_editor->getAnimationSpeedMultiplier(); // The "speed multiplier" is a "duration divider"
}

} // namespace app
