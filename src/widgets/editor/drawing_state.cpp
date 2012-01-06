/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "widgets/editor/drawing_state.h"

#include "gui/message.h"
#include "gui/system.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "tools/tool_loop.h"
#include "tools/tool_loop_manager.h"
#include "widgets/editor/editor.h"

#include <allegro.h>

static tools::ToolLoopManager::Pointer pointer_from_msg(Message* msg)
{
  tools::ToolLoopManager::Pointer::Button button =
    (msg->mouse.right ? tools::ToolLoopManager::Pointer::Right:
     (msg->mouse.middle ? tools::ToolLoopManager::Pointer::Middle:
                          tools::ToolLoopManager::Pointer::Left));

  return tools::ToolLoopManager::Pointer(msg->mouse.x, msg->mouse.y, button);
}

DrawingState::DrawingState(tools::ToolLoop* toolLoop, Editor* editor, Message* msg)
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
  delete m_toolLoopManager;
  delete m_toolLoop;
  m_toolLoopManager = NULL;
  m_toolLoop = NULL;
}

bool DrawingState::onMouseDown(Editor* editor, Message* msg)
{
  // Drawing loop
  ASSERT(m_toolLoopManager != NULL);

  // Notify the mouse button down to the tool loop manager.
  m_toolLoopManager->pressButton(pointer_from_msg(msg));

  // Cancel drawing loop
  if (m_toolLoopManager->isCanceled()) {
    m_toolLoopManager->releaseLoop(pointer_from_msg(msg));

    delete m_toolLoopManager;
    m_toolLoopManager = NULL;

    // Change to standby state
    editor->backToPreviousState();
    editor->releaseMouse();
  }

  return true;
}

bool DrawingState::onMouseUp(Editor* editor, Message* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  // Notify the release of the mouse button to the tool loop manager.
  if (m_toolLoopManager->releaseButton(pointer_from_msg(msg)))
    return true;

  m_toolLoopManager->releaseLoop(pointer_from_msg(msg));

  // Back to standby state.
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool DrawingState::onMouseMove(Editor* editor, Message* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  acquire_bitmap(ji_screen);

  // Hide the drawing cursor
  editor->hideDrawingCursor();

  // Infinite scroll
  editor->controlInfiniteScroll(msg);

  // Hide the cursor again
  editor->hideDrawingCursor();

  // notify mouse movement to the tool
  ASSERT(m_toolLoopManager != NULL);
  m_toolLoopManager->movement(pointer_from_msg(msg));

  // draw the cursor again
  editor->showDrawingCursor();

  release_bitmap(ji_screen);
  return true;
}

bool DrawingState::onSetCursor(Editor* editor)
{
  if (m_toolLoop->getInk()->isEyedropper()) {
    editor->hideDrawingCursor();
    jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
  }
  else {
    jmouse_set_cursor(JI_CURSOR_NULL);
    editor->showDrawingCursor();
  }
  return true;
}

bool DrawingState::onKeyDown(Editor* editor, Message* msg)
{
  if (editor->processKeysToSetZoom(msg->key.scancode))
    return true;

  // When we are drawing, we "eat" all pressed keys.
  return true;
}

bool DrawingState::onKeyUp(Editor* editor, Message* msg)
{
  return true;
}

bool DrawingState::onUpdateStatusBar(Editor* editor)
{
  // The status bar is updated by ToolLoopImpl::updateStatusBar()
  // method called by the ToolLoopManager.
  return false;
}
