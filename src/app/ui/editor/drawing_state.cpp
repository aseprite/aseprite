// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "app/ui/editor/editor_render.h"
#include "app/ui/editor/glue.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/scoped_value.h"
#include "doc/layer.h"
#include "ui/message.h"
#include "ui/system.h"

// TODO remove these headers and make this state observable by the timeline.
#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline/timeline.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace app {

using namespace ui;

static int get_delay_interval_for_tool_loop(tools::ToolLoop* toolLoop)
{
  if (toolLoop->getTracePolicy() == tools::TracePolicy::Last) {
    // We use the delayed mouse movement for tools like Line,
    // Rectangle, etc. (tools that use the last mouse position for its
    // shape, so we can discard intermediate positions).
    return 5;
  }
  else {
    // Without delay for freehand-like tools
    return 0;
  }
}

DrawingState::DrawingState(Editor* editor,
                           tools::ToolLoop* toolLoop,
                           const DrawingType type)
  : m_editor(editor)
  , m_type(type)
  , m_delayedMouseMove(this, editor,
                       get_delay_interval_for_tool_loop(toolLoop))
  , m_toolLoop(toolLoop)
  , m_toolLoopManager(new tools::ToolLoopManager(toolLoop))
  , m_mouseMoveReceived(false)
  , m_mousePressedReceived(false)
  , m_processScrollChange(true)
{
  m_beforeCmdConn =
    UIContext::instance()->BeforeCommandExecution.connect(
      &DrawingState::onBeforeCommandExecution, this);
}

DrawingState::~DrawingState()
{
  destroyLoop(nullptr);
}

void DrawingState::initToolLoop(Editor* editor,
                                const ui::MouseMessage* msg,
                                const tools::Pointer& pointer)
{
  if (msg)
    m_delayedMouseMove.onMouseDown(msg);
  else
    m_delayedMouseMove.initSpritePos(gfx::PointF(pointer.point()));

  Tileset* tileset = m_toolLoop->getDstTileset();

  // For selection inks we don't use a "the selected layer" for
  // preview purposes, because we want the selection feedback to be at
  // the top of all layers.
  Layer* previewLayer = (m_toolLoop->getInk()->isSelection() ? nullptr:
                                                               m_toolLoop->getLayer());

  // Prepare preview image (the destination image will be our preview
  // in the tool-loop time, so we can see what we are drawing)
  editor->renderEngine().setPreviewImage(
    previewLayer,
    m_toolLoop->getFrame(),
    tileset ? nullptr: m_toolLoop->getDstImage(),
    tileset,
    m_toolLoop->getCelOrigin(),
    (previewLayer &&
     previewLayer->isImage() ?
     static_cast<LayerImage*>(m_toolLoop->getLayer())->blendMode():
     doc::BlendMode::NEG_BW)); // To preview the selection ink we use the negative black & white blender

  ASSERT(!m_toolLoopManager->isCanceled());

  m_velocity.reset();
  m_lastPointer = pointer;
  m_mouseDownPos = (msg ? msg->position():
                          editor->editorToScreen(pointer.point()));
  m_mouseDownTime = base::current_tick();

  m_toolLoopManager->prepareLoop(pointer);
  m_toolLoopManager->pressButton(pointer);

  ASSERT(!m_toolLoopManager->isCanceled());

  editor->captureMouse();
}

void DrawingState::sendMovementToToolLoop(const tools::Pointer& pointer)
{
  ASSERT(m_toolLoopManager);
  m_lastPointer = pointer;
  m_toolLoopManager->movement(pointer);
}

void DrawingState::notifyToolLoopModifiersChange(Editor* editor)
{
  if (!m_toolLoopManager->isCanceled())
    m_toolLoopManager->notifyToolLoopModifiersChange();
}

void DrawingState::onBeforePopState(Editor* editor)
{
  m_beforeCmdConn.disconnect();
  StandbyState::onBeforePopState(editor);
}

bool DrawingState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  // Drawing loop
  ASSERT(m_toolLoopManager != NULL);

  if (!editor->hasCapture())
    editor->captureMouse();

  tools::Pointer pointer = pointer_from_msg(editor, msg,
                                            m_velocity.velocity());
  m_lastPointer = pointer;

  m_delayedMouseMove.onMouseDown(msg);

  // Check if this drawing state was started with a Shift+Pencil tool
  // and now the user pressed the right button to draw the straight
  // line with the background color.
  bool recreateLoop = false;
  if (m_type == DrawingType::LineFreehand &&
      !m_mousePressedReceived) {
    recreateLoop = true;
  }

  m_mousePressedReceived = true;

  // Notify the mouse button down to the tool loop manager.
  m_toolLoopManager->pressButton(pointer);

  // Store the isCanceled flag, because destroyLoopIfCanceled might
  // destroy the tool loop manager.
  ASSERT(m_toolLoopManager);
  const bool isCanceled = m_toolLoopManager->isCanceled();

  // The user might have canceled by the tool loop clicking with the
  // secondary mouse button.
  destroyLoopIfCanceled(editor);

  // In this case, the user canceled the straight line preview
  // (Shift+Pencil) pressing the right-click (other mouse button). Now
  // we have to restart the loop calling
  // checkStartDrawingStraightLine() with the right-button.
  if (recreateLoop && isCanceled) {
    ASSERT(!m_toolLoopManager);
    checkStartDrawingStraightLine(editor, msg, &pointer);
  }

  return true;
}

bool DrawingState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_toolLoopManager != NULL);

  m_lastPointer = pointer_from_msg(editor, msg, m_velocity.velocity());
  m_delayedMouseMove.onMouseUp(msg);

  // Selection tools with Replace mode are cancelled with a simple click.
  // ("one point" controller selection tool i.e. the magic wand, and
  // selection tools with Add or Subtract mode aren't cancelled with
  // one click).
  if (!m_toolLoop->getInk()->isSelection() ||
      m_toolLoop->getController()->isOnePoint() ||
      !canInterpretMouseMovementAsJustOneClick() ||
      // In case of double-click (to select tiles) we don't want to
      // deselect if the mouse is not moved. In this case the tile
      // will be selected anyway even if the mouse is not moved.
      m_type == DrawingType::SelectTiles ||
      (editor->getToolLoopModifiers() != tools::ToolLoopModifiers::kReplaceSelection &&
       editor->getToolLoopModifiers() != tools::ToolLoopModifiers::kIntersectSelection)) {
    // Notify the release of the mouse button to the tool loop
    // manager. This is the correct way to say "the user finishes the
    // drawing trace correctly".
    if (m_toolLoopManager->releaseButton(m_lastPointer))
      return true;
  }

  destroyLoop(editor);

  // Back to standby state.
  editor->backToPreviousState();
  editor->releaseMouse();

  // Update the timeline. TODO make this state observable by the timeline.
  App::instance()->timeline()->updateUsingEditor(editor);

  // Restart again? Here we handle the case to draw multiple lines
  // using Shift+click with the Pencil tool. When we release the mouse
  // button, if the Shift key is pressed, the whole ToolLoop starts
  // again.
  if (Preferences::instance().editor.straightLinePreview())
    checkStartDrawingStraightLine(editor, msg, &m_lastPointer);

  return true;
}

bool DrawingState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  // It's needed to avoid some glitches with brush boundaries.
  //
  // TODO we should be able to avoid this if we correctly invalidate
  // the BrushPreview::m_clippingRegion
  HideBrushPreview hide(editor->brushPreview());

  // Don't process onScrollChange() messages if autoScroll() changes
  // the scroll.
  base::ScopedValue disableScroll(m_processScrollChange, false);

  // Update velocity sensor.
  m_velocity.updateWithDisplayPoint(msg->position());

  // Update pointer with new mouse position
  m_lastPointer = tools::Pointer(gfx::Point(m_delayedMouseMove.spritePos()),
                                 m_velocity.velocity(),
                                 button_from_msg(msg),
                                 msg->pointerType(),
                                 msg->pressure());

  // Indicate that we've received a real mouse movement event here
  // (used in the Rectangular Marquee to deselect when we just do a
  // simple click without moving the mouse).
  m_mouseMoveReceived = true;
  gfx::Point delta = (msg->position() - m_mouseDownPos);
  m_mouseMaxDelta.x = std::max(m_mouseMaxDelta.x, std::abs(delta.x));
  m_mouseMaxDelta.y = std::max(m_mouseMaxDelta.y, std::abs(delta.y));

  // Use DelayedMouseMove for tools like line, rectangle, etc. (that
  // use the only the last mouse position) to filter out rapid mouse
  // movement.
  m_delayedMouseMove.onMouseMove(msg);
  return true;
}

void DrawingState::onCommitMouseMove(Editor* editor,
                                     const gfx::PointF& spritePos)
{
  if (m_toolLoop &&
      m_toolLoopManager &&
      !m_toolLoopManager->isCanceled()) {
    handleMouseMovement();
  }
}

bool DrawingState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  if (m_toolLoop->getInk()->isEyedropper()) {
    auto theme = skin::SkinTheme::get(editor);
    editor->showMouseCursor(
      kCustomCursor, theme->cursors.eyedropper());
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
    // We accept some commands...
    if (command->id() == CommandId::Zoom() ||
        command->id() == CommandId::Undo() ||
        command->id() == CommandId::Redo() ||
        command->id() == CommandId::Cancel()) {
      UIContext::instance()->executeCommandFromMenuOrShortcut(command, params);
      return true;
    }
  }

  // Return true when we cannot execute commands (true = the onKeyDown
  // event was used, so the key is not used to run a command).
  return !canExecuteCommands();
}

bool DrawingState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  // Cancel loop pressing Esc key...
  if (msg->scancode() == ui::kKeyEsc ||
      // Cancel "Shift on freehand" line preview when the Shift key is
      // released and the user didn't press the mouse button.
      (m_type == DrawingType::LineFreehand &&
       !m_mousePressedReceived &&
       !editor->startStraightLineWithFreehandTool(nullptr))) {
    m_toolLoopManager->cancel();
  }

  // The user might have canceled the tool loop pressing the 'Esc' key.
  destroyLoopIfCanceled(editor);
  return true;
}

bool DrawingState::onScrollChange(Editor* editor)
{
  if (m_processScrollChange) {
    gfx::Point mousePos = editor->mousePosInDisplay();

    // Update velocity sensor.
    m_velocity.updateWithDisplayPoint(mousePos); // TODO add scroll as velocity?

    m_lastPointer = tools::Pointer(editor->screenToEditor(mousePos),
                                   m_velocity.velocity(),
                                   m_lastPointer.button(),
                                   m_lastPointer.type(),
                                   m_lastPointer.pressure());
    handleMouseMovement();
  }
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

bool DrawingState::getGridBounds(Editor* editor, gfx::Rect& gridBounds)
{
  if (m_toolLoop) {
    gridBounds = m_toolLoop->getGridBounds();
    return true;
  }
  else
    return false;
}

void DrawingState::handleMouseMovement()
{
  // Notify mouse movement to the tool
  ASSERT(m_toolLoopManager);
  m_toolLoopManager->movement(m_lastPointer);
}

bool DrawingState::canInterpretMouseMovementAsJustOneClick()
{
  // If the user clicked (pressed and released the mouse button) in
  // less than 250 milliseconds in "the same place" (inside a 7 pixels
  // rectangle actually, to detect stylus shake).
  return
    !m_mouseMoveReceived ||
    (m_mouseMaxDelta.x < 4 &&
     m_mouseMaxDelta.y < 4 &&
     (base::current_tick() - m_mouseDownTime < 250));
}

bool DrawingState::canExecuteCommands()
{
  // Returning true here means that the user can trigger commands with
  // keyboard shortcuts. In our case we want to be able to use
  // keyboard shortcuts only when the Shift key was pressed to run a
  // command (e.g. Shift+N), not to draw a straight line from the
  // pencil (freehand) tool.
  return (m_type == DrawingType::LineFreehand &&
          !m_mousePressedReceived);
}

void DrawingState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  if (!m_toolLoop)
    return;

  if (canExecuteCommands() ||
      // Undo/Redo/Cancel will cancel the ToolLoop
      ev.command()->id() == CommandId::Undo() ||
      ev.command()->id() == CommandId::Redo() ||
      ev.command()->id() == CommandId::Cancel()) {
    if (!canExecuteCommands()) {
      // Cancel the execution of Undo/Redo/Cancel because we've
      // simulated it here
      ev.cancel();
    }

    m_toolLoopManager->cancel();
    destroyLoopIfCanceled(m_editor);
  }
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

  if (m_toolLoopManager)
    m_toolLoopManager->end();

  m_toolLoopManager.reset(nullptr);
  m_toolLoop.reset(nullptr);

  app_rebuild_documents_tabs();
}

} // namespace app
