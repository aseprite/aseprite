// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_pixels_state.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/commands/cmd_flip.h"
#include "app/commands/cmd_move_mask.h"
#include "app/commands/cmd_rotate.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/transform_handles.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "base/bind.h"
#include "base/pi.h"
#include "base/unique_ptr.h"
#include "doc/algorithm/flip_image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <cstring>

namespace app {

using namespace ui;

MovingPixelsState::MovingPixelsState(Editor* editor, MouseMessage* msg, PixelsMovementPtr pixelsMovement, HandleType handle)
  : m_editor(editor)
  , m_observingEditor(false)
  , m_discarded(false)
{
  // MovingPixelsState needs a selection tool to avoid problems
  // sharing the extra cel between the drawing cursor preview and the
  // pixels movement/transformation preview.
  //ASSERT(!editor->getCurrentEditorInk()->isSelection());

  UIContext* context = UIContext::instance();

  m_pixelsMovement = pixelsMovement;

  if (handle != NoHandle) {
    gfx::Point pt = editor->screenToEditor(msg->position());
    m_pixelsMovement->catchImage(pt, handle);

    editor->captureMouse();
  }

  // Setup transparent mode/mask color
  if (Preferences::instance().selection.autoOpaque()) {
    Preferences::instance().selection.opaque(
      editor->layer()->isBackground());
  }
  onTransparentColorChange();

  // Hook BeforeCommandExecution signal so we know if the user wants
  // to execute other command, so we can drop pixels.
  m_ctxConn =
    context->BeforeCommandExecution.connect(&MovingPixelsState::onBeforeCommandExecution, this);

  // Listen to any change to the transparent color from the ContextBar.
  m_opaqueConn =
    Preferences::instance().selection.opaque.AfterChange.connect(
      base::Bind<void>(&MovingPixelsState::onTransparentColorChange, this));
  m_transparentConn =
    Preferences::instance().selection.transparentColor.AfterChange.connect(
      base::Bind<void>(&MovingPixelsState::onTransparentColorChange, this));

  // Add the current editor as filter for key message of the manager
  // so we can catch the Enter key, and avoid to execute the
  // PlayAnimation command.
  m_editor->manager()->addMessageFilter(kKeyDownMessage, m_editor);
  m_editor->manager()->addMessageFilter(kKeyUpMessage, m_editor);
  m_editor->addObserver(this);
  m_observingEditor = true;

  ContextBar* contextBar = App::instance()->getMainWindow()->getContextBar();
  contextBar->updateForMovingPixels();
  contextBar->addObserver(this);
}

MovingPixelsState::~MovingPixelsState()
{
  ContextBar* contextBar = App::instance()->getMainWindow()->getContextBar();
  contextBar->removeObserver(this);
  contextBar->updateForCurrentTool();

  m_pixelsMovement.reset(NULL);

  removeAsEditorObserver();
  m_editor->manager()->removeMessageFilter(kKeyDownMessage, m_editor);
  m_editor->manager()->removeMessageFilter(kKeyUpMessage, m_editor);

  m_editor->document()->generateMaskBoundaries();
}

void MovingPixelsState::translate(const gfx::Point& delta)
{
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  m_pixelsMovement->catchImageAgain(gfx::Point(0, 0), MoveHandle);
  m_pixelsMovement->moveImage(delta, PixelsMovement::NormalMovement);
  m_pixelsMovement->dropImageTemporarily();
}

void MovingPixelsState::rotate(double angle)
{
  m_pixelsMovement->rotate(angle);
}

void MovingPixelsState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);

  update_screen_for_document(editor->document());
}

EditorState::LeaveAction MovingPixelsState::onLeaveState(Editor* editor, EditorState* newState)
{
  LOG("MovingPixels: leave state\n");

  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // If we are changing to another state, we've to drop the image.
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  // Drop pixels if we are changing to a non-temporary state (a
  // temporary state is something like ScrollingState).
  if (!newState || !newState->isTemporalState()) {
    if (!m_discarded) {
      try {
        m_pixelsMovement->dropImage();
      }
      catch (const LockedDocumentException& ex) {
        // This is one of the worst possible scenarios. We want to
        // drop pixels because we're leaving this state (e.g. the user
        // changed the current frame/layer, so we came from
        // onBeforeFrameChanged) and we weren't able to drop those
        // pixels.
        //
        // TODO this problem should be caught before we reach this
        // state, or this problem should cancel the frame/layer
        // change.
        Console::showException(ex);
      }
    }

    editor->document()->resetTransformation();

    m_pixelsMovement.reset(NULL);

    editor->releaseMouse();

    // Redraw the document without the transformation handles.
    editor->document()->notifyGeneralUpdate();

    return DiscardState;
  }
  else {
    editor->releaseMouse();
    return KeepState;
  }
}

void MovingPixelsState::onCurrentToolChange(Editor* editor)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  tools::Tool* current_tool = editor->getCurrentEditorTool();

  // If the user changed the tool when he/she is moving pixels,
  // we have to drop the pixels only if the new tool is not selection...
  if (m_pixelsMovement &&
      (!current_tool->getInk(0)->isSelection() ||
       !current_tool->getInk(1)->isSelection())) {
    // We have to drop pixels
    dropPixels();
  }
}

bool MovingPixelsState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // Set this editor as the active one and setup the ContextBar for
  // moving pixels. This is needed in case that the user is working
  // with a couple of Editors, in one is moving pixels and the other
  // one not.
  UIContext* ctx = UIContext::instance();
  ctx->setActiveView(editor->getDocumentView());

  ContextBar* contextBar = App::instance()->getMainWindow()->getContextBar();
  contextBar->updateForMovingPixels();

  // Start scroll loop
  if (checkForScroll(editor, msg) || checkForZoom(editor, msg))
    return true;

  // Call the eyedropper command
  tools::Ink* clickedInk = editor->getCurrentEditorInk();
  if (clickedInk->isEyedropper()) {
    callEyedropper(editor);
    return true;
  }

  Decorator* decorator = static_cast<Decorator*>(editor->decorator());
  Document* document = editor->document();

  // Transform selected pixels
  if (document->isMaskVisible() &&
      decorator->getTransformHandles(editor)) {
    TransformHandles* transfHandles = decorator->getTransformHandles(editor);

    // Get the handle covered by the mouse.
    HandleType handle = transfHandles->getHandleAtPoint(editor,
                                                        msg->position(),
                                                        getTransformation(editor));

    if (handle != NoHandle) {
      // Re-catch the image
      m_pixelsMovement->catchImageAgain(
        editor->screenToEditor(msg->position()), handle);

      editor->captureMouse();
      return true;
    }
  }

  // Start "moving pixels" loop
  if (editor->isInsideSelection() && (msg->left() ||
                                      msg->right())) {
    // In case that the user is pressing the copy-selection keyboard shortcut.
    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
    if ((customization) &&
        int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection)) {
      // Stamp the pixels to create the copy.
      m_pixelsMovement->stampImage();
    }

    // Re-catch the image
    m_pixelsMovement->catchImageAgain(
      editor->screenToEditor(msg->position()), MoveHandle);

    editor->captureMouse();
    return true;
  }
  // End "moving pixels" loop
  else {
    // Drop pixels (e.g. to start drawing)
    dropPixels();
  }

  // Use StandbyState implementation
  return StandbyState::onMouseDown(editor, msg);
}

bool MovingPixelsState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // Drop the image temporarily in this location (where the user releases the mouse)
  m_pixelsMovement->dropImageTemporarily();

  // Redraw the new pivot location.
  editor->invalidate();

  editor->releaseMouse();
  return true;
}

bool MovingPixelsState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // If there is a button pressed
  if (m_pixelsMovement->isDragging()) {
    // Auto-scroll
    gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir, false);

    // Get the position of the mouse in the sprite
    gfx::Point spritePos = editor->screenToEditor(mousePos);

    // Get the customization for the pixels movement (snap to grid, angle snap, etc.).
    KeyContext keyContext = KeyContext::Normal;
    switch (m_pixelsMovement->handle()) {
      case MoveHandle:
        keyContext = KeyContext::TranslatingSelection;
        break;
      case ScaleNWHandle:
      case ScaleNHandle:
      case ScaleNEHandle:
      case ScaleWHandle:
      case ScaleEHandle:
      case ScaleSWHandle:
      case ScaleSHandle:
      case ScaleSEHandle:
        keyContext = KeyContext::ScalingSelection;
        break;
      case RotateNWHandle:
      case RotateNHandle:
      case RotateNEHandle:
      case RotateWHandle:
      case RotateEHandle:
      case RotateSWHandle:
      case RotateSHandle:
      case RotateSEHandle:
        keyContext = KeyContext::RotatingSelection;
        break;
    }

    PixelsMovement::MoveModifier moveModifier = PixelsMovement::NormalMovement;
    KeyAction action = editor->getCustomizationDelegate()
      ->getPressedKeyAction(keyContext);

    if (int(action & KeyAction::SnapToGrid))
      moveModifier |= PixelsMovement::SnapToGridMovement;

    if (int(action & KeyAction::AngleSnap))
      moveModifier |= PixelsMovement::AngleSnapMovement;

    if (int(action & KeyAction::MaintainAspectRatio))
      moveModifier |= PixelsMovement::MaintainAspectRatioMovement;

    if (int(action & KeyAction::LockAxis))
      moveModifier |= PixelsMovement::LockAxisMovement;

    // Invalidate handles
    Decorator* decorator = static_cast<Decorator*>(editor->decorator());
    TransformHandles* transfHandles = decorator->getTransformHandles(editor);
    transfHandles->invalidateHandles(editor, m_pixelsMovement->getTransformation());

    // Drag the image to that position
    m_pixelsMovement->moveImage(spritePos, moveModifier);

    editor->updateStatusBar();
    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingPixelsState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // Move selection
  if (m_pixelsMovement->isDragging()) {
    editor->showMouseCursor(kMoveCursor);
    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onSetCursor(editor, mouseScreenPos);
}

bool MovingPixelsState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  ASSERT(m_pixelsMovement);
  if (!isActiveEditor())
    return false;
  ASSERT(editor == m_editor);

  if (msg->scancode() == kKeyEnter || // TODO make this key customizable
      msg->scancode() == kKeyEnterPad ||
      msg->scancode() == kKeyEsc) {
    dropPixels();

    // The escape key drop pixels and deselect the mask.
    if (msg->scancode() == kKeyEsc) { // TODO make this key customizable
      Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
      UIContext::instance()->executeCommand(cmd);
    }

    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onKeyDown(editor, msg);
}

bool MovingPixelsState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  ASSERT(m_pixelsMovement);
  if (!isActiveEditor())
    return false;
  ASSERT(editor == m_editor);

  // Use StandbyState implementation
  return StandbyState::onKeyUp(editor, msg);
}

bool MovingPixelsState::onUpdateStatusBar(Editor* editor)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  const gfx::Transformation& transform(getTransformation(editor));
  gfx::Size imageSize = m_pixelsMovement->getInitialImageSize();

  StatusBar::instance()->setStatusText
    (100, "Moving Pixels - Pos %d %d, Size %d %d, Orig: %3d %3d (%.02f%% %.02f%%), Angle %.1f",
     transform.bounds().x, transform.bounds().y,
     transform.bounds().w, transform.bounds().h,
     imageSize.w, imageSize.h,
     (double)transform.bounds().w*100.0/imageSize.w,
     (double)transform.bounds().h*100.0/imageSize.h,
     180.0 * transform.angle() / PI);

  return true;
}

bool MovingPixelsState::acceptQuickTool(tools::Tool* tool)
{
  return
    (!m_pixelsMovement ||
     tool->getInk(0)->isSelection() ||
     tool->getInk(0)->isEyedropper() ||
     tool->getInk(0)->isScrollMovement() ||
     tool->getInk(0)->isZoom());
}

// Before executing any command, we drop the pixels (go back to standby).
void MovingPixelsState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  Command* command = ev.command();

  LOG("MovingPixelsState::onBeforeCommandExecution %s\n", command->id().c_str());

  // If the command is for other editor, we don't drop pixels.
  if (!isActiveEditor())
    return;

  // We don't need to drop the pixels if a MoveMaskCommand of Content is executed.
  if (MoveMaskCommand* moveMaskCmd = dynamic_cast<MoveMaskCommand*>(ev.command())) {
    if (moveMaskCmd->getTarget() == MoveMaskCommand::Content) {
      // Do not drop pixels
      return;
    }
  }
  else if ((command->id() == CommandId::Zoom) ||
           (command->id() == CommandId::Scroll)) {
    // Do not drop pixels
    return;
  }
  // Intercept the "Cut" or "Copy" command to handle them locally
  // with the current m_pixelsMovement data.
  else if (command->id() == CommandId::Cut ||
           command->id() == CommandId::Copy ||
           command->id() == CommandId::Clear) {
    // Copy the floating image to the clipboard on Cut/Copy.
    if (command->id() != CommandId::Clear) {
      Document* document = m_editor->document();
      base::UniquePtr<Image> floatingImage;
      base::UniquePtr<Mask> floatingMask;
      m_pixelsMovement->getDraggedImageCopy(floatingImage, floatingMask);

      clipboard::copy_image(floatingImage.get(),
                            floatingMask.get(),
                            document->sprite()->palette(m_editor->frame()));
    }

    // Clear floating pixels on Cut/Clear.
    if (command->id() != CommandId::Copy) {
      // Discard the dragged image.
      m_pixelsMovement->discardImage();
      m_discarded = true;

      // Quit from MovingPixelsState, back to standby.
      m_editor->backToPreviousState();
    }

    // Cancel the command, we've simulated it.
    ev.cancel();
    return;
  }
  // Flip Horizontally/Vertically commands are handled manually to
  // avoid dropping the floating region of pixels.
  else if (command->id() == CommandId::Flip) {
    if (FlipCommand* flipCommand = dynamic_cast<FlipCommand*>(command)) {
      m_pixelsMovement->flipImage(flipCommand->getFlipType());

      ev.cancel();
      return;
    }
  }
  // Rotate is quite simple, we can add the angle to the current transformation.
  else if (command->id() == CommandId::Rotate) {
    if (RotateCommand* rotate = dynamic_cast<RotateCommand*>(command)) {
      if (rotate->flipMask()) {
        m_pixelsMovement->rotate(rotate->angle());

        ev.cancel();
        return;
      }
    }
  }

  if (m_pixelsMovement)
    dropPixels();
}

void MovingPixelsState::onDestroyEditor(Editor* editor)
{
  // TODO we should call ~MovingPixelsState(), we should delete the
  //      whole "m_statesHistory" when an editor is deleted.
  removeAsEditorObserver();
}

void MovingPixelsState::onBeforeFrameChanged(Editor* editor)
{
  if (!isActiveDocument())
    return;

  if (m_pixelsMovement)
    dropPixels();
}

void MovingPixelsState::onBeforeLayerChanged(Editor* editor)
{
  if (!isActiveDocument())
    return;

  if (m_pixelsMovement)
    dropPixels();
}

void MovingPixelsState::onTransparentColorChange()
{
  bool opaque = Preferences::instance().selection.opaque();
  setTransparentColor(
    opaque,
    opaque ?
      app::Color::fromMask():
      Preferences::instance().selection.transparentColor());
}

void MovingPixelsState::onDropPixels(ContextBarObserver::DropAction action)
{
  if (!isActiveEditor())
    return;

  switch (action) {

    case ContextBarObserver::DropPixels:
      dropPixels();
      break;

    case ContextBarObserver::CancelDrag:
      m_pixelsMovement->discardImage(false);
      m_discarded = true;

      // Quit from MovingPixelsState, back to standby.
      m_editor->backToPreviousState();
      break;
  }
}

void MovingPixelsState::setTransparentColor(bool opaque, const app::Color& color)
{
  ASSERT(m_pixelsMovement);

  Layer* layer = m_editor->layer();
  ASSERT(layer);

  try {
    m_pixelsMovement->setMaskColor(
      opaque, color_utils::color_for_target_mask(color, ColorTarget(layer)));
  }
  catch (const LockedDocumentException& ex) {
    Console::showException(ex);
  }
}

void MovingPixelsState::dropPixels()
{
  LOG("MovingPixels: drop pixels\n");

  // Just change to default state (StandbyState generally). We'll
  // receive an onLeaveState() event after this call.
  m_editor->backToPreviousState();
}

gfx::Transformation MovingPixelsState::getTransformation(Editor* editor)
{
  return m_pixelsMovement->getTransformation();
}

bool MovingPixelsState::isActiveDocument() const
{
  Document* doc = UIContext::instance()->activeDocument();
  return (m_editor->document() == doc);
}

bool MovingPixelsState::isActiveEditor() const
{
  Editor* targetEditor = UIContext::instance()->activeEditor();
  return (targetEditor == m_editor);
}

void MovingPixelsState::removeAsEditorObserver()
{
  if (m_observingEditor) {
    m_observingEditor = false;
    m_editor->removeObserver(this);
  }
}

} // namespace app
