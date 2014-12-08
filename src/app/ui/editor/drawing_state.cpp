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

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_loop.h"
#include "app/tools/tool_loop_manager.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/scoped_cursor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui_context.h"
#include "doc/blend.h"
#include "ui/message.h"
#include "ui/system.h"

// TODO remove these headers and make this state observable by the timeline.
#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"

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

DrawingState::DrawingState(tools::ToolLoop* toolLoop)
  : m_toolLoop(toolLoop)
  , m_toolLoopManager(new tools::ToolLoopManager(toolLoop))
  , m_mouseMoveReceived(false)
{
}

DrawingState::~DrawingState()
{
  destroyLoop();
}

void DrawingState::initToolLoop(Editor* editor, MouseMessage* msg)
{
  HideShowDrawingCursor hideShow(editor);

  m_toolLoopManager->prepareLoop(pointer_from_msg(msg));
  m_toolLoopManager->pressButton(pointer_from_msg(msg));

  editor->captureMouse();
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

  // Selection tools are cancelled with a simple click (only "one
  // point" controller selection tools aren't cancelled with one click,
  // i.e. the magic wand).
  if (!m_toolLoop->getInk()->isSelection() ||
      m_toolLoop->getController()->isOnePoint() ||
      m_mouseMoveReceived) {
    // Notify the release of the mouse button to the tool loop
    // manager. This is the correct way to say "the user finishes the
    // drawing trace correctly".
    if (m_toolLoopManager->releaseButton(pointer_from_msg(msg)))
      return true;
  }

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

  m_mouseMoveReceived = true;

  // Hide the drawing cursor
  HideShowDrawingCursor hideShow(editor);

  // Infinite scroll
  gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir, true);

  // Hide the cursor again
  editor->hideDrawingCursor();

  // notify mouse movement to the tool
  ASSERT(m_toolLoopManager != NULL);
  m_toolLoopManager
    ->movement(tools::ToolLoopManager::Pointer(mousePos.x, mousePos.y,
                                               button_from_msg(msg)));

  return true;
}

bool DrawingState::onSetCursor(Editor* editor)
{
  if (m_toolLoop->getInk()->isEyedropper()) {
    editor->hideDrawingCursor();
    ui::set_mouse_cursor(kEyedropperCursor);
  }
  else {
    ui::set_mouse_cursor(kNoCursor);
    editor->showDrawingCursor();
  }
  return true;
}

bool DrawingState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  if (msg->repeat() == 0)
    m_toolLoopManager->pressKey(msg->scancode());

  Command* command = NULL;
  Params* params = NULL;
  if (KeyboardShortcuts::instance()
        ->getCommandFromKeyMessage(msg, &command, &params)) {
    // We accept zoom commands.
    if (strcmp(command->short_name(), CommandId::Zoom) == 0) {
      UIContext::instance()->executeCommand(command, params);
    }
  }

  // When we are drawing, we "eat" all pressed keys.
  return true;
}

bool DrawingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  m_toolLoopManager->releaseKey(msg->scancode());
  return true;
}

bool DrawingState::onUpdateStatusBar(Editor* editor)
{
  // The status bar is updated by ToolLoopImpl::updateStatusBar()
  // method called by the ToolLoopManager.
  return false;
}

void DrawingState::onExposeSpritePixels(const gfx::Region& rgn)
{
  if (m_toolLoop)
    m_toolLoop->validateDstImage(rgn);
}

void DrawingState::destroyLoop()
{
  delete m_toolLoopManager;
  delete m_toolLoop;
  m_toolLoopManager = NULL;
  m_toolLoop = NULL;
}

} // namespace app
