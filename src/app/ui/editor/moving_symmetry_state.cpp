// Aseprite
// Copyright (C) 2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_symmetry_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "ui/message.h"

namespace app {

using namespace ui;

MovingSymmetryState::MovingSymmetryState(Editor* editor, MouseMessage* msg,
                                         app::gen::SymmetryMode mode,
                                         Option<int>& symmetryAxis)
  : m_symmetryMode(mode)
  , m_symmetryAxis(symmetryAxis)
  , m_symmetryAxisStart(symmetryAxis())
{
  m_mouseStart = editor->screenToEditor(msg->position());
  editor->captureMouse();
}

MovingSymmetryState::~MovingSymmetryState()
{
}

bool MovingSymmetryState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSymmetryState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;
  int pos = 0;

  switch (m_symmetryMode) {
    case app::gen::SymmetryMode::HORIZONTAL:
      pos = m_symmetryAxisStart + delta.x;
      pos = MID(1, pos, editor->sprite()->width()-1);
      break;
    case app::gen::SymmetryMode::VERTICAL:
      pos = m_symmetryAxisStart + delta.y;
      pos = MID(1, pos, editor->sprite()->height()-1);
      break;
  }
  m_symmetryAxis(pos);

  // Redraw the editor.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSymmetryState::onUpdateStatusBar(Editor* editor)
{
  if (m_symmetryMode == app::gen::SymmetryMode::HORIZONTAL)
    StatusBar::instance()->setStatusText
      (0, "Left %3d Right %3d", m_symmetryAxis(),
       editor->sprite()->width() - m_symmetryAxis());
  else
    StatusBar::instance()->setStatusText
      (0, "Top %3d Bottom %3d", m_symmetryAxis(),
       editor->sprite()->height() - m_symmetryAxis());

  return true;
}

} // namespace app
