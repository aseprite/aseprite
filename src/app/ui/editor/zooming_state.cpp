// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/zooming_state.h"

#include "app/app.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "gfx/rect.h"
#include "doc/sprite.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;

ZoomingState::ZoomingState()
  : m_startZoom(1, 1)
  , m_moved(false)
{
}

ZoomingState::~ZoomingState()
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
  render::Zoom zoom(1, 1);
  double scale = m_startZoom.scale() + pt.x / 16.0;
  if (scale < 1.0)
    scale = 1.0 / -(scale-2.0);
  zoom = render::Zoom::fromScale(scale);

  editor->setZoomAndCenterInMouse(
    zoom, m_startPos, Editor::ZoomBehavior::MOUSE);

  m_moved = true;
  return true;
}

bool ZoomingState::onSetCursor(Editor* editor)
{
  editor->hideDrawingCursor();
  ui::set_mouse_cursor(kMagnifierCursor);
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
