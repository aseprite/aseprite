/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app/ui/editor/scrolling_state.h"

#include "app/app.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "gfx/rect.h"
#include "raster/sprite.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;
  
ScrollingState::ScrollingState()
{
}

ScrollingState::~ScrollingState()
{
}

bool ScrollingState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  return true;
}

bool ScrollingState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool ScrollingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  View* view = View::getView(editor);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll = view->getViewScroll();

  editor->setEditorScroll(scroll.x+jmouse_x(1)-jmouse_x(0),
                          scroll.y+jmouse_y(1)-jmouse_y(0), true);

  jmouse_control_infinite_scroll(vp);

  int x, y;
  editor->screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
  StatusBar::instance()->setStatusText
    (0, "Pos %3d %3d (Size %3d %3d)", x, y,
     editor->getSprite()->getWidth(),
     editor->getSprite()->getHeight());

  return true;
}

bool ScrollingState::onMouseWheel(Editor* editor, MouseMessage* msg)
{
  return false;
}

bool ScrollingState::onSetCursor(Editor* editor)
{
  editor->hideDrawingCursor();
  jmouse_set_cursor(kScrollCursor);
  return true;
}

bool ScrollingState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool ScrollingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool ScrollingState::onUpdateStatusBar(Editor* editor)
{
  return false;
}

} // namespace app
