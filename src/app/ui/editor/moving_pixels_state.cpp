// Aseprite
// Copyright (C) 2019 Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "base/gcd.h"
#include "base/pi.h"
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
  : m_pixelsMovement(pixelsMovement)
  , m_editor(editor)
  , m_observingEditor(false)
  , m_discarded(false)
{
  // MovingPixelsState needs a selection tool to avoid problems
  // sharing the extra cel between the drawing cursor preview and the
  // pixels movement/transformation preview.
  //ASSERT(!editor->getCurrentEditorInk()->isSelection());

  UIContext* context = UIContext::instance();

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
  m_editor->add_observer(this);
  m_observingEditor = true;

  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels();
  contextBar->add_observer(this);
}

MovingPixelsState::~MovingPixelsState()
{
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->remove_observer(this);
  contextBar->updateForActiveTool();

  removePixelsMovement();
  removeAsEditorObserver();

  m_editor->manager()->removeMessageFilter(kKeyDownMessage, m_editor);
  m_editor->manager()->removeMessageFilter(kKeyUpMessage, m_editor);

  m_editor->document()->generateMaskBoundaries();
}

void MovingPixelsState::translate(const gfx::Point& delta)
{
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  m_pixelsMovement->catchImageAgain(gfx::Point(0, 0), MovePixelsHandle);
  m_pixelsMovement->moveImage(delta, PixelsMovement::NormalMovement);
  m_pixelsMovement->dropImageTemporarily();
}

void MovingPixelsState::rotate(double angle)
{
  m_pixelsMovement->rotate(angle);
}

void MovingPixelsState::flip(doc::algorithm::FlipType flipType)
{
  m_pixelsMovement->flipImage(flipType);
}

void MovingPixelsState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);

  update_screen_for_document(editor->document());
}

EditorState::LeaveAction MovingPixelsState::onLeaveState(Editor* editor, EditorState* newState)
{
  TRACE("MOVPIXS: onLeaveState\n");

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
      catch (const LockedDocException& ex) {
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

    removePixelsMovement();

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

void MovingPixelsState::onActiveToolChange(Editor* editor, tools::Tool* tool)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // If the user changed the tool when he/she is moving pixels,
  // we have to drop the pixels only if the new tool is not selection...
  if (m_pixelsMovement) {
    // We don't want to drop pixels in case the user change the tool
    // for scrolling/zooming/picking colors.
    if ((!tool->getInk(0)->isSelection() ||
         !tool->getInk(1)->isSelection()) &&
        (!tool->getInk(0)->isScrollMovement() ||
         !tool->getInk(1)->isScrollMovement()) &&
        (!tool->getInk(0)->isZoom() ||
         !tool->getInk(1)->isZoom()) &&
        (!tool->getInk(0)->isEyedropper() ||
         !tool->getInk(1)->isEyedropper())) {
      // We have to drop pixels
      dropPixels();
    }
    // If we've temporarily gone to a non-selection tool and now we're
    // back, we've just to update the context bar to show the "moving
    // pixels" controls (e.g. OK/Cancel movement buttons).
    else if (tool->getInk(0)->isSelection() ||
             tool->getInk(1)->isSelection()) {
      ContextBar* contextBar = App::instance()->contextBar();
      contextBar->updateForMovingPixels();
    }
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
  ctx->setActiveView(editor->getDocView());

  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels();

  // Start scroll loop
  if (editor->checkForScroll(msg) ||
      editor->checkForZoom(msg))
    return true;

  // Call the eyedropper command
  tools::Ink* clickedInk = editor->getCurrentEditorInk();
  if (clickedInk->isEyedropper()) {
    callEyedropper(editor, msg);
    return true;
  }

  Decorator* decorator = static_cast<Decorator*>(editor->decorator());
  Doc* document = editor->document();

  // Transform selected pixels
  if (document->isMaskVisible() &&
      decorator->getTransformHandles(editor) &&
      (!Preferences::instance().selection.modifiersDisableHandles() ||
       msg->modifiers() == kKeyNoneModifier)) {
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

  // Start "moving pixels" loop. Here we check only for left-click as
  // right-click can be used to deselect/subtract selection, so we
  // should drop the selection in this later case.
  if (editor->isInsideSelection() && msg->left()) {
    // In case that the user is pressing the copy-selection keyboard shortcut.
    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
    if ((customization) &&
        int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection)) {
      // Stamp the pixels to create the copy.
      m_pixelsMovement->stampImage();
    }

    // Re-catch the image
    m_pixelsMovement->catchImageAgain(
      editor->screenToEditor(msg->position()), MovePixelsHandle);

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
    gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir);

    // Get the position of the mouse in the sprite
    gfx::Point spritePos = editor->screenToEditor(mousePos);

    // Get the customization for the pixels movement (snap to grid, angle snap, etc.).
    KeyContext keyContext = KeyContext::Normal;
    switch (m_pixelsMovement->handle()) {
      case MovePixelsHandle:
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

    if (int(action & KeyAction::ScaleFromCenter))
      moveModifier |= PixelsMovement::ScaleFromPivot;

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
      Command* cmd = Commands::instance()->byId(CommandId::DeselectMask());
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

  const Transformation& transform(getTransformation(editor));
  gfx::Size imageSize = m_pixelsMovement->getInitialImageSize();

  int w = int(transform.bounds().w);
  int h = int(transform.bounds().h);
  int gcd = base::gcd(w, h);
  StatusBar::instance()->setStatusText
    (100, ":pos: %d %d :size: %3d %3d :selsize: %d %d [%.02f%% %.02f%%] :angle: %.1f :aspect_ratio: %2d : %2d",
     int(transform.bounds().x),
     int(transform.bounds().y),
     imageSize.w,
     imageSize.h,
     w,
     h,
     (double)w*100.0/imageSize.w,
     (double)h*100.0/imageSize.h,
     180.0 * transform.angle() / PI,
     w/gcd,
     h/gcd);

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

  TRACE("MOVPIXS: onBeforeCommandExecution %s\n", command->id().c_str());

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
  // Don't drop pixels if the user zooms/scrolls/picks a color
  // using commands.
  else if ((command->id() == CommandId::Zoom()) ||
           (command->id() == CommandId::Scroll()) ||
           (command->id() == CommandId::Eyedropper()) ||
           // DiscardBrush is used by Eyedropper command
           (command->id() == CommandId::DiscardBrush())) {
    // Do not drop pixels
    return;
  }
  // Intercept the "Cut" or "Copy" command to handle them locally
  // with the current m_pixelsMovement data.
  else if (command->id() == CommandId::Cut() ||
           command->id() == CommandId::Copy() ||
           command->id() == CommandId::Clear()) {
    // Copy the floating image to the clipboard on Cut/Copy.
    if (command->id() != CommandId::Clear()) {
      Doc* document = m_editor->document();
      std::unique_ptr<Image> floatingImage;
      std::unique_ptr<Mask> floatingMask;
      m_pixelsMovement->getDraggedImageCopy(floatingImage, floatingMask);

      clipboard::copy_image(floatingImage.get(),
                            floatingMask.get(),
                            document->sprite()->palette(m_editor->frame()));
    }

    // Clear floating pixels on Cut/Clear.
    if (command->id() != CommandId::Copy()) {
      m_pixelsMovement->trim();

      // Should we keep the mask after an Edit > Clear command?
      auto keepMask = PixelsMovement::DontKeepMask;
      if (command->id() == CommandId::Clear() &&
          Preferences::instance().selection.keepSelectionAfterClear()) {
        keepMask = PixelsMovement::KeepMask;
      }

      // Discard the dragged image.
      m_pixelsMovement->discardImage(PixelsMovement::CommitChanges, keepMask);
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
  else if (command->id() == CommandId::Flip()) {
    if (FlipCommand* flipCommand = dynamic_cast<FlipCommand*>(command)) {
      m_pixelsMovement->flipImage(flipCommand->getFlipType());

      ev.cancel();
      return;
    }
  }
  // Rotate is quite simple, we can add the angle to the current transformation.
  else if (command->id() == CommandId::Rotate()) {
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
  ASSERT(m_pixelsMovement);

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
      m_pixelsMovement->discardImage(PixelsMovement::DontCommitChanges);
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
  catch (const LockedDocException& ex) {
    Console::showException(ex);
  }
}

void MovingPixelsState::dropPixels()
{
  TRACE("MOVPIXS: dropPixels\n");

  // Just change to default state (StandbyState generally). We'll
  // receive an onLeaveState() event after this call.
  m_editor->backToPreviousState();
}

Transformation MovingPixelsState::getTransformation(Editor* editor)
{
  // m_pixelsMovement can be null in the final onMouseDown(), after we
  // called dropPixels() and we're just going to the previous state.
  if (m_pixelsMovement)
    return m_pixelsMovement->getTransformation();
  else
    return StandbyState::getTransformation(editor);
}

bool MovingPixelsState::isActiveDocument() const
{
  Doc* doc = UIContext::instance()->activeDocument();
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
    m_editor->remove_observer(this);
  }
}

void MovingPixelsState::removePixelsMovement()
{
  m_pixelsMovement.reset(nullptr);
  m_ctxConn.disconnect();
  m_opaqueConn.disconnect();
  m_transparentConn.disconnect();
}

} // namespace app
