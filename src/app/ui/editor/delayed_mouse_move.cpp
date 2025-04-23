// Aseprite
// Copyright (C) 2022-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/delayed_mouse_move.h"

#include "app/ui/editor/editor.h"
#include "base/time.h"
#include "ui/message.h"

namespace app {

static const gfx::Point kNoPosReceived(std::numeric_limits<int>::min(),
                                       std::numeric_limits<int>::min());

DelayedMouseMove::DelayedMouseMove(DelayedMouseMoveDelegate* delegate,
                                   Editor* editor,
                                   const int interval)
  : m_delegate(delegate)
  , m_editor(editor)
  , m_timer(interval)
  , m_mouseMoveReceived(false)
  , m_mouseDownPos(kNoPosReceived)
  , m_mouseDownTime(base::current_tick())
  , m_spritePos(std::numeric_limits<float>::min(), std::numeric_limits<float>::min())
{
  ASSERT(m_delegate);
  m_timer.Tick.connect([this] { commitMouseMove(); });
}

void DelayedMouseMove::reset()
{
  m_mouseMoveReceived = false;
  m_mouseDownPos = kNoPosReceived;
}

void DelayedMouseMove::initSpritePos(const gfx::PointF& pos)
{
  m_spritePos = pos;
}

void DelayedMouseMove::onMouseDown(const ui::MouseMessage* msg)
{
  if (m_mouseDownPos == kNoPosReceived) {
    m_mouseDownPos = msg->position();
  }

  updateSpritePos(msg);
}

bool DelayedMouseMove::onMouseMove(const ui::MouseMessage* msg)
{
  // Indicate that we've received a real mouse movement event here
  // (used in the Rectangular Marquee to deselect when we just do a
  // simple click without moving the mouse).
  m_mouseMoveReceived = true;
  gfx::Point delta = (msg->position() - m_mouseDownPos);
  m_mouseMaxDelta.x = std::max(m_mouseMaxDelta.x, std::abs(delta.x));
  m_mouseMaxDelta.y = std::max(m_mouseMaxDelta.y, std::abs(delta.y));

  if (!updateSpritePos(msg))
    return false;

  if (!m_timer.isRunning()) {
    if (m_timer.interval() > 0) {
      m_timer.start();
    }
    else {
      // Commit immediately
      commitMouseMove();
    }
  }
  return true;
}

void DelayedMouseMove::onMouseUp(const ui::MouseMessage* msg)
{
  if (updateSpritePos(msg))
    commitMouseMove();
}

void DelayedMouseMove::stopTimer()
{
  if (m_timer.isRunning())
    m_timer.stop();
}

bool DelayedMouseMove::canInterpretMouseMovementAsJustOneClick() const
{
  return !m_mouseMoveReceived || (m_mouseMaxDelta.x < 4 && m_mouseMaxDelta.y < 4 &&
                                  (base::current_tick() - m_mouseDownTime < 250));
}

void DelayedMouseMove::commitMouseMove()
{
  if (m_timer.isRunning())
    m_timer.stop();

  try {
    m_delegate->onCommitMouseMove(m_editor, spritePos());
  }
  catch (const std::exception& ex) {
    m_editor->showUnhandledException(ex, nullptr);
  }
}

const gfx::PointF& DelayedMouseMove::spritePos() const
{
  ASSERT(m_spritePos.x != std::numeric_limits<float>::min() &&
         m_spritePos.y != std::numeric_limits<float>::min());
  return m_spritePos;
}

bool DelayedMouseMove::updateSpritePos(const ui::MouseMessage* msg)
{
  // The autoScroll() function controls the "infinite scroll" when we
  // touch the viewport borders.
  const gfx::Point mousePos = m_editor->autoScroll(msg, AutoScroll::MouseDir);
  const gfx::PointF spritePos = m_editor->screenToEditorF(mousePos);

  // Avoid redrawing everything if the position in the canvas didn't
  // change.
  if (m_spritePos != spritePos) {
    m_spritePos = spritePos;
    return true;
  }
  else
    return false;
}

} // namespace app
