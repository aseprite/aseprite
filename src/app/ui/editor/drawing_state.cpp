// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/glue.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "ui/message.h"
#include "ui/system.h"

// TODO remove these headers and make this state observable by the timeline.
#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline/timeline.h"

#include <cstring>

namespace app {

using namespace ui;

DrawingState::DrawingState(tools::ToolLoop* toolLoop)
  : m_toolLoop(toolLoop)
  , m_toolLoopManager(new tools::ToolLoopManager(toolLoop))
  , m_mouseMoveReceived(false)
{
}

DrawingState::~DrawingState()
{
  destroyLoop(nullptr);
}

void DrawingState::initToolLoop(Editor* editor, MouseMessage* msg)
{
  // Prepare preview image (the destination image will be our preview
  // in the tool-loop time, so we can see what we are drawing)
  editor->renderEngine().setPreviewImage(
    m_toolLoop->getLayer(),
    m_toolLoop->getFrame(),
    m_toolLoop->getDstImage(),
    m_toolLoop->getCelOrigin(),
    (m_toolLoop->getLayer() &&
     m_toolLoop->getLayer()->isImage() ?
     static_cast<LayerImage*>(m_toolLoop->getLayer())->blendMode():
     BlendMode::NEG_BW));

  tools::Pointer pointer;
  bool movement = false;

  if (m_toolLoop->getController()->isFreehand() &&
      m_toolLoop->getInk()->isPaint() &&
      (editor->getCustomizationDelegate()
         ->getPressedKeyAction(KeyContext::FreehandTool) & KeyAction::StraightLineFromLastPoint) == KeyAction::StraightLineFromLastPoint &&
      editor->document()->lastDrawingPoint() != app::Document::NoLastDrawingPoint()) {
    pointer = tools::Pointer(editor->document()->lastDrawingPoint(),
                             button_from_msg(msg));
    movement = true;
  }
  else {
    pointer = pointer_from_msg(editor, msg);
  }

  m_toolLoopManager->prepareLoop(pointer);
  m_toolLoopManager->pressButton(pointer);

  // This first movement is done when the user pressed Shift+click in
  // a freehand tool to draw a straight line.
  if (movement) {
    pointer = pointer_from_msg(editor, msg);
    m_toolLoopManager->movement(pointer);
  }

  editor->captureMouse();
}

void DrawingState::notifyToolLoopModifiersChange(Editor* editor)
{
  if (!m_toolLoopManager->isCanceled())
    m_toolLoopManager->notifyToolLoopModifiersChange();
}

bool DrawingState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  // Drawing loop
  ASSERT(m_toolLoopManager != NULL);

  // Notify the mouse button down to the tool loop manager.
  m_toolLoopManager->pressButton(pointer_from_msg(editor, msg));

  // The user might have canceled by the tool loop clicking with the
  // secondary mouse button.
  destroyLoopIfCanceled(editor);
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
    if (m_toolLoopManager->releaseButton(pointer_from_msg(editor, msg)))
      return true;
  }

  destroyLoop(editor);

  // Back to standby state.
  editor->backToPreviousState();
  editor->releaseMouse();

  // Update the timeline. TODO make this state observable by the timeline.
  App::instance()->timeline()->updateUsingEditor(editor);
  return true;
}

bool DrawingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  m_mouseMoveReceived = true;

  // It's needed to avoid some glitches with brush boundaries.
  //
  // TODO we should be able to avoid this if we correctly invalidate
  // the BrushPreview::m_clippingRegion
  HideBrushPreview hide(editor->brushPreview());

  // Infinite scroll
  gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir);
  tools::Pointer pointer(editor->screenToEditor(mousePos),
                         button_from_msg(msg));

  // Notify mouse movement to the tool
  ASSERT(m_toolLoopManager != NULL);
  m_toolLoopManager->movement(pointer);

  return true;
}

bool DrawingState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (m_toolLoop->getInk()->isEyedropper()) {
    editor->showMouseCursor(
      kCustomCursor, skin::SkinTheme::instance()->cursors.eyedropper());
  }
  else {
    editor->showBrushPreview(mouseScreenPos);
  }
  return true;
}

bool DrawingState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  Command* command = NULL;
  Params params;
  if (KeyboardShortcuts::instance()
        ->getCommandFromKeyMessage(msg, &command, &params)) {
    // We accept zoom commands.
    if (command->id() == CommandId::Zoom)
      UIContext::instance()->executeCommand(command, params);
  }

  // When we are drawing, we "eat" all pressed keys.
  return true;
}

bool DrawingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  if (msg->scancode() == ui::kKeyEsc)
    m_toolLoop->cancel();

  // The user might have canceled the tool loop pressing the 'Esc' key.
  destroyLoopIfCanceled(editor);
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

void DrawingState::destroyLoopIfCanceled(Editor* editor)
{
  // Cancel drawing loop
  if (m_toolLoopManager->isCanceled()) {
    destroyLoop(editor);

    // Change to standby state
    editor->backToPreviousState();
    editor->releaseMouse();
  }
}

void DrawingState::destroyLoop(Editor* editor)
{
  if (editor)
    editor->renderEngine().removePreviewImage();

  if (m_toolLoop)
    m_toolLoop->commitOrRollback();

  delete m_toolLoopManager;
  delete m_toolLoop;
  m_toolLoopManager = nullptr;
  m_toolLoop = nullptr;

  app_rebuild_documents_tabs();
}

} // namespace app
