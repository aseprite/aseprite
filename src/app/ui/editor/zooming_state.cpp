/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

    editor->setZoomAndCenterInMouse(zoom,
      msg->position(), Editor::kCofiguredZoomBehavior);
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool ZoomingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point pt = (msg->position() - m_startPos);
  int length = ABS(pt.x);
  render::Zoom zoom = m_startZoom;

  if (length > 0) {
    if (pt.x > 0) {
      while (length--)
        zoom.in();
    }
    else {
      while (length--)
        zoom.out();
    }
  }

  editor->setZoomAndCenterInMouse(zoom,
    m_startPos, Editor::kDontCenterOnZoom);

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
