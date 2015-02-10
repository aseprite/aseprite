/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/ui/editor/navigate_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/editor/scrolling_state.h"

namespace app {

using namespace ui;

NavigateState::NavigateState()
{
}

bool NavigateState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  if (editor->hasCapture())
    return true;

  // UIContext* context = UIContext::instance();
  // // When an editor is clicked the current view is changed.
  // context->setActiveView(editor->getDocumentView());

  // Start scroll loop
  EditorStatePtr newState(new ScrollingState());
  editor->setState(newState);
  newState->onMouseDown(editor, msg);
  return true;
}

bool NavigateState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->releaseMouse();
  return true;
}

bool NavigateState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  editor->moveDrawingCursor();
  editor->updateStatusBar();
  return true;
}

bool NavigateState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool NavigateState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return false;
}

} // namespace app
