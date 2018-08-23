// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/zooming_state.h"

#include "app/app.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "os/display.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

#include <cmath>

namespace app {

using namespace ui;

ZoomingState::ZoomingState()
  : m_startZoom(1, 1)
  , m_moved(false)
{
}

bool ZoomingState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  m_startPos = msg->position();
  m_startZoom = editor->zoom();

  editor->captureMouse();
  return true;
}

bool ZoomingState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  if (!m_moved) {
    render::Zoom zoom = editor->zoom();

    if (msg->left())
      zoom.in();
    else if (msg->right())
      zoom.out();

    editor->setZoomAndCenterInMouse(
      zoom, msg->position(), Editor::ZoomBehavior::MOUSE);
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool ZoomingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point pt = (msg->position() - m_startPos);
  int threshold = 8 * guiscale() * editor->manager()->getDisplay()->scale();

  if (m_moved || std::sqrt(pt.x*pt.x + pt.y*pt.y) > threshold) {
    m_moved = true;

    int newScale = m_startZoom.linearScale() + pt.x / threshold;
    render::Zoom newZoom = render::Zoom::fromLinearScale(newScale);

    editor->setZoomAndCenterInMouse(
      newZoom, m_startPos, Editor::ZoomBehavior::MOUSE);
  }
  return true;
}

bool ZoomingState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  editor->showMouseCursor(
    kCustomCursor, skin::SkinTheme::instance()->cursors.magnifier());
  return true;
}

bool ZoomingState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool ZoomingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool ZoomingState::onUpdateStatusBar(Editor* editor)
{
  return false;
}

} // namespace app
