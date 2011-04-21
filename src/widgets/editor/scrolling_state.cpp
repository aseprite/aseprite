/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "widgets/editor/scrolling_state.h"

#include "app.h"
#include "gfx/rect.h"
#include "gui/message.h"
#include "gui/system.h"
#include "gui/view.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

ScrollingState::ScrollingState()
{
}

ScrollingState::~ScrollingState()
{
}

bool ScrollingState::onMouseDown(Editor* editor, Message* msg)
{
  return true;
}

bool ScrollingState::onMouseUp(Editor* editor, Message* msg)
{
  editor->setState(editor->getDefaultState());
  editor->releaseMouse();
  return true;
}

bool ScrollingState::onMouseMove(Editor* editor, Message* msg)
{
  View* view = View::getView(editor);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll = view->getViewScroll();

  editor->setEditorScroll(scroll.x+jmouse_x(1)-jmouse_x(0),
			  scroll.y+jmouse_y(1)-jmouse_y(0), true);

  jmouse_control_infinite_scroll(vp);

  int x, y;
  editor->screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);
  app_get_statusbar()->setStatusText
    (0, "Pos %3d %3d (Size %3d %3d)", x, y,
     editor->getSprite()->getWidth(),
     editor->getSprite()->getHeight());

  return true;
}

bool ScrollingState::onMouseWheel(Editor* editor, Message* msg)
{
  return false;
}

bool ScrollingState::onSetCursor(Editor* editor)
{
  editor->hideDrawingCursor();
  jmouse_set_cursor(JI_CURSOR_SCROLL);
  return true;
}

bool ScrollingState::onKeyDown(Editor* editor, Message* msg)
{
  return false;
}

bool ScrollingState::onKeyUp(Editor* editor, Message* msg)
{
  return false;
}

bool ScrollingState::onUpdateStatusBar(Editor* editor)
{
  return false;
}
