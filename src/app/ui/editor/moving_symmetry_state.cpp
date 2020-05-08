// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_symmetry_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "base/clamp.h"
#include "fmt/format.h"
#include "ui/message.h"

#include <cmath>

namespace app {

using namespace ui;

MovingSymmetryState::MovingSymmetryState(Editor* editor, MouseMessage* msg,
                                         app::gen::SymmetryMode mode,
                                         Option<double>& symmetryAxis)
  : m_symmetryMode(mode)
  , m_symmetryAxis(symmetryAxis)
  , m_symmetryAxisStart(symmetryAxis())
{
  m_mouseStart = editor->screenToEditorF(msg->position());
  editor->captureMouse();
}

bool MovingSymmetryState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingSymmetryState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::PointF newCursorPos = editor->screenToEditorF(msg->position());
  gfx::PointF delta = newCursorPos - m_mouseStart;
  double pos = 0.0;

  switch (m_symmetryMode) {
    case app::gen::SymmetryMode::HORIZONTAL:
      pos = m_symmetryAxisStart + delta.x;
      pos = std::round(pos*2.0)/2.0;
      pos = base::clamp(pos, 1.0, editor->sprite()->width()-1.0);
      break;
    case app::gen::SymmetryMode::VERTICAL:
      pos = m_symmetryAxisStart + delta.y;
      pos = std::round(pos*2.0)/2.0;
      pos = base::clamp(pos, 1.0, editor->sprite()->height()-1.0);
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
    StatusBar::instance()->setStatusText(
      0, fmt::format("Left {:3.1f} Right {:3.1f}",
                     m_symmetryAxis(),
                     double(editor->sprite()->width()) - m_symmetryAxis()));
  else
    StatusBar::instance()->setStatusText(
      0, fmt::format("Top {:3.1f} Bottom {:3.1f}",
                     m_symmetryAxis(),
                     double(editor->sprite()->height()) - m_symmetryAxis()));

  return true;
}

} // namespace app
