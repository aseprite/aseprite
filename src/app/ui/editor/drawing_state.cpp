/* Aseprite
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

#include "app/ui/editor/drawing_state.h"

#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_loop.h"
#include "app/tools/tool_loop_manager.h"
#include "app/ui/editor/editor.h"
#include "ui/message.h"
#include "ui/system.h"

// TODO remove these headers and make this state observable by the timeline.
#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"

#include <allegro.h>

namespace app {

using namespace ui;

static tools::ToolLoopManager::Pointer::Button button_from_msg(MouseMessage* msg)
{
  return
    (msg->right() ? tools::ToolLoopManager::Pointer::Right:
     (msg->middle() ? tools::ToolLoopManager::Pointer::Middle:
                      tools::ToolLoopManager::Pointer::Left));
}

static tools::ToolLoopManager::Pointer pointer_from_msg(MouseMessage* msg)
{
  return
    tools::ToolLoopManager::Pointer(msg->position().x, msg->position().y, button_from_msg(msg));
}

DrawingState::DrawingState(tools::ToolLoop* toolLoop, Editor* editor, MouseMessage* msg)
  : m_toolLoop(toolLoop)
  , m_toolLoopManager(new tools::ToolLoopManager(toolLoop))
{
  // Hide the cursor (mainly to clean the pen preview)
  editor->hideDrawingCursor();

  m_toolLoopManager->prepareLoop(pointer_from_msg(msg));
  m_toolLoopManager->pressButton(pointer_from_msg(msg));

  // Show drawing cursor again (without pen preview in this case)
  editor->showDrawingCursor();

  editor->captureMouse();
}

DrawingState::~DrawingState()
{
  destroyLoop();
}

bool DrawingState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  // Drawing loop
  ASSERT(m_toolLoopManager != NULL);

  // Notify the mouse button down to the tool loop manager.
  m_toolLoopManager->pressButton(pointer_from_msg(msg));

  // Cancel drawing loop
  if (m_toolLoopManager->isCanceled()) {
    m_toolLoopManager->releaseLoop(pointer_from_msg(msg));
    destroyLoop();

    // Change to standby state
    editor->backToPreviousState();
    editor->releaseMouse();
  }

  return true;
}

bool DrawingState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  // Notify the release of the mouse button to the tool loop manager.
  if (m_toolLoopManager->releaseButton(pointer_from_msg(msg)))
    return true;

  m_toolLoopManager->releaseLoop(pointer_from_msg(msg));
  destroyLoop();

  // Back to standby state.
  editor->backToPreviousState();
  editor->releaseMouse();

  // Update the timeline. TODO make this state observable by the timeline.
  App::instance()->getMainWindow()->getTimeline()->updateUsingEditor(editor);
  return true;
}

bool DrawingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  acquire_bitmap(ji_screen);

  // Hide the drawing cursor
  editor->hideDrawingCursor();

  // Infinite scroll
  gfx::Point mousePos = editor->controlInfiniteScroll(msg);

  // Hide the cursor again
  editor->hideDrawingCursor();

  // notify mouse movement to the tool
  ASSERT(m_toolLoopManager != NULL);
  m_toolLoopManager
    ->movement(tools::ToolLoopManager::Pointer(mousePos.x, mousePos.y,
                                               button_from_msg(msg)));

  // draw the cursor again
  editor->showDrawingCursor();

  release_bitmap(ji_screen);
  return true;
}

bool DrawingState::onSetCursor(Editor* editor)
{
  if (m_toolLoop->getInk()->isEyedropper()) {
    editor->hideDrawingCursor();
    jmouse_set_cursor(kEyedropperCursor);
  }
  else {
    jmouse_set_cursor(kNoCursor);
    editor->showDrawingCursor();
  }
  return true;
}

bool DrawingState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  if (editor->processKeysToSetZoom(msg))
    return true;

  // When we are drawing, we "eat" all pressed keys.
  return true;
}

bool DrawingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  return true;
}

bool DrawingState::onUpdateStatusBar(Editor* editor)
{
  // The status bar is updated by ToolLoopImpl::updateStatusBar()
  // method called by the ToolLoopManager.
  return false;
}

void DrawingState::destroyLoop()
{
  delete m_toolLoopManager;
  delete m_toolLoop;
  m_toolLoopManager = NULL;
  m_toolLoop = NULL;
}

} // namespace app
