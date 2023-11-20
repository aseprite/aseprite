// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define MOVPIXS_TRACE(...) // TRACE(__VA_ARGS__)

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
#include "app/commands/move_thing.h"
#include "app/console.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/transformation.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/pixels_movement.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/transform_handles.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/layer_utils.h"
#include "base/gcd.h"
#include "base/pi.h"
#include "doc/algorithm/flip_image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "gfx/rect.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/view.h"

#include <cstring>

namespace app {

using namespace ui;

MovingPixelsState::MovingPixelsState(Editor* editor,
                                     MouseMessage* msg,
                                     PixelsMovementPtr pixelsMovement,
                                     HandleType handle)
  : m_pixelsMovement(pixelsMovement)
  , m_delayedMouseMove(this, editor, 5)
  , m_editor(editor)
  , m_observingEditor(false)
  , m_discarded(false)
  , m_renderTimer(50)
{
  m_pixelsMovement->setDelegate(this);

  // MovingPixelsState needs a selection tool to avoid problems
  // sharing the extra cel between the drawing cursor preview and the
  // pixels movement/transformation preview.
  //ASSERT(!editor->getCurrentEditorInk()->isSelection());

  UIContext* context = UIContext::instance();

  if (handle != NoHandle) {
    gfx::Point pt = editor->screenToEditor(msg->position());
    m_pixelsMovement->catchImage(gfx::PointF(pt), handle);

    editor->captureMouse();
  }

  // Setup transparent mode/mask color
  if (Preferences::instance().selection.autoOpaque()) {
    Preferences::instance().selection.opaque(
      editor->layer()->isBackground());
  }
  onTransparentColorChange();

  m_renderTimer.Tick.connect([this]{ onRenderTimer(); });

  // Hook BeforeCommandExecution signal so we know if the user wants
  // to execute other command, so we can drop pixels.
  m_ctxConn =
    context->BeforeCommandExecution.connect(&MovingPixelsState::onBeforeCommandExecution, this);

  // Listen to any change to the transparent color from the ContextBar.
  m_opaqueConn =
    Preferences::instance().selection.opaque.AfterChange.connect(
      [this]{ onTransparentColorChange(); });
  m_transparentConn =
    Preferences::instance().selection.transparentColor.AfterChange.connect(
      [this]{ onTransparentColorChange(); });

  // Add the current editor as filter for key message of the manager
  // so we can catch the Enter key, and avoid to execute the
  // PlayAnimation command.
  m_editor->manager()->addMessageFilter(kKeyDownMessage, m_editor);
  m_editor->manager()->addMessageFilter(kKeyUpMessage, m_editor);
  m_editor->add_observer(this);
  m_observingEditor = true;

  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels(getTransformation(editor));
  contextBar->add_observer(this);

  App::instance()->mainWindow()->getTimeline()->add_observer(this);
}

MovingPixelsState::~MovingPixelsState()
{
  App::instance()->mainWindow()->getTimeline()->remove_observer(this);

  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->remove_observer(this);
  contextBar->updateForActiveTool();

  removePixelsMovement();
  removeAsEditorObserver();
  m_renderTimer.stop();

  m_editor->manager()->removeMessageFilter(kKeyDownMessage, m_editor);
  m_editor->manager()->removeMessageFilter(kKeyUpMessage, m_editor);

  m_editor->document()->generateMaskBoundaries();
}

void MovingPixelsState::translate(const gfx::PointF& delta)
{
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  m_pixelsMovement->catchImageAgain(gfx::PointF(0, 0), MovePixelsHandle);
  m_pixelsMovement->moveImage(delta, PixelsMovement::NormalMovement);
  m_pixelsMovement->dropImageTemporarily();
  m_editor->updateStatusBar();
}

void MovingPixelsState::rotate(double angle)
{
  m_pixelsMovement->rotate(angle);
  m_editor->updateStatusBar();
}

void MovingPixelsState::flip(doc::algorithm::FlipType flipType)
{
  m_pixelsMovement->flipImage(flipType);
  m_editor->updateStatusBar();
}

void MovingPixelsState::shift(int dx, int dy)
{
  m_pixelsMovement->shift(dx, dy);
  m_editor->updateStatusBar();
}

void MovingPixelsState::updateTransformation(const Transformation& t)
{
  m_pixelsMovement->setTransformation(t);
  m_editor->updateStatusBar();
}

void MovingPixelsState::onEnterState(Editor* editor)
{
  StandbyState::onEnterState(editor);

  // Get the active key action so we can lock the FineControl action
  // until we release/press again the Ctrl key just in case if we
  // started moving the pixels with the Ctrl key pressed.
  m_lockedKeyAction = getCurrentKeyAction();

  update_screen_for_document(editor->document());
}

void MovingPixelsState::onEditorGotFocus(Editor* editor)
{
  ContextBar* contextBar = App::instance()->contextBar();
  // Make the DropPixelsField widget visible again in the ContextBar
  // when we are back to an editor in MovingPixelsState. Without this
  // we would see the SelectionModeField instead which doesn't make
  // sense on MovingPixelsState).
  contextBar->updateForMovingPixels(getTransformation(editor));
}

EditorState::LeaveAction MovingPixelsState::onLeaveState(Editor* editor, EditorState* newState)
{
  MOVPIXS_TRACE("MOVPIXS: onLeaveState\n");

  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  onRenderTimer();

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
      contextBar->updateForMovingPixels(getTransformation(editor));
    }
  }
}

bool MovingPixelsState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  m_delayedMouseMove.onMouseDown(msg);

  // Set this editor as the active one and setup the ContextBar for
  // moving pixels. This is needed in case that the user is working
  // with a couple of Editors, in one is moving pixels and the other
  // one not.
  UIContext* ctx = UIContext::instance();
  ctx->setActiveView(editor->getDocView());

  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels(getTransformation(editor));

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
      if (layer_is_locked(editor))
        return true;

      // Re-catch the image
      gfx::Point pt = editor->screenToEditor(msg->position());
      m_pixelsMovement->catchImageAgain(gfx::PointF(pt), handle);

      editor->captureMouse();
      return true;
    }
  }

  // Start "moving pixels" loop. Here we check only for left-click as
  // right-click can be used to deselect/subtract selection, so we
  // should drop the selection in this later case.
  if (editor->isInsideSelection() && msg->left()) {
    if (layer_is_locked(editor))
      return true;

    // In case that the user is pressing the copy-selection keyboard shortcut.
    if (auto customization = editor->getCustomizationDelegate()) {
      if (int(customization->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::CopySelection)) {
        // Stamp the pixels to create the copy.
        m_pixelsMovement->stampImage();
      }

      // Set the locked key action again to lock the FineControl until
      // with press Ctrl key again.
      m_lockedKeyAction = getCurrentKeyAction();
    }

    // Re-catch the image
    m_pixelsMovement->catchImageAgain(
      editor->screenToEditorF(msg->position()), MovePixelsHandle);

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

  m_delayedMouseMove.onMouseUp(msg);

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
    if (m_delayedMouseMove.onMouseMove(msg))
      m_renderTimer.start();
    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

void MovingPixelsState::onCommitMouseMove(Editor* editor,
                                          const gfx::PointF& spritePos)
{
  ASSERT(m_pixelsMovement);

  if (!m_pixelsMovement->isDragging())
    return;

  m_pixelsMovement->setFastMode(true);

  const KeyAction action = getCurrentKeyAction();
  PixelsMovement::MoveModifier moveModifier = PixelsMovement::NormalMovement;
  bool snapToGrid = (Preferences::instance().selection.snapToGrid() &&
                     m_editor->docPref().grid.snap());
  if (bool(action & KeyAction::SnapToGrid))
    snapToGrid = !snapToGrid;

  if (snapToGrid)
    moveModifier |= PixelsMovement::SnapToGridMovement;

  if (int(action & KeyAction::AngleSnap))
    moveModifier |= PixelsMovement::AngleSnapMovement;

  if (int(action & KeyAction::MaintainAspectRatio))
    moveModifier |= PixelsMovement::MaintainAspectRatioMovement;

  if (int(action & KeyAction::ScaleFromCenter))
    moveModifier |= PixelsMovement::ScaleFromPivot;

  if (int(action & KeyAction::LockAxis))
    moveModifier |= PixelsMovement::LockAxisMovement;

  // Enable the FineControl only if we've already released and pressed
  // the key again (because it )
  if (int(action & KeyAction::FineControl) &&
      int(m_lockedKeyAction & KeyAction::FineControl) == 0) {
    moveModifier |= PixelsMovement::FineControl;
  }

  // Invalidate handles
  Decorator* decorator = static_cast<Decorator*>(m_editor->decorator());
  TransformHandles* transfHandles = decorator->getTransformHandles(m_editor);
  const Transformation& transformation = m_pixelsMovement->getTransformation();
  transfHandles->invalidateHandles(m_editor, transformation);

  // Drag the image to that position
  m_pixelsMovement->moveImage(spritePos, moveModifier);

  // Update context bar and status bar
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels(transformation);

  m_editor->updateStatusBar();
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

  // Reset the locked action just to indicate that we can use the
  // FineControl now (e.g. if we pressed another modifier key).
  m_lockedKeyAction = KeyAction::None;

  if (msg->scancode() == kKeyEnter || // TODO make this key customizable
      msg->scancode() == kKeyEnterPad ||
      msg->scancode() == kKeyEsc) {
    dropPixels();

    // The escape key drop pixels and deselect the mask.
    if (msg->scancode() == kKeyEsc) { // TODO make this key customizable
      Command* cmd = Commands::instance()->byId(CommandId::DeselectMask());
      UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
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

  // Reset the locked action just to indicate that we can use the
  // FineControl now (e.g. if we release the Ctrl key).
  m_lockedKeyAction = KeyAction::None;

  // Use StandbyState implementation
  return StandbyState::onKeyUp(editor, msg);
}

bool MovingPixelsState::onUpdateStatusBar(Editor* editor)
{
  MOVPIXS_TRACE("MOVPIXS: onUpdateStatusBar (%p)\n", m_pixelsMovement.get());

  ASSERT(m_pixelsMovement);
  ASSERT(editor == m_editor);

  // We've received a crash report where this is nullptr when
  // MovingPixelsState::onLeaveState() generates a general update
  // notification (notifyGeneralUpdate()) just after the
  // m_pixelsMovement is deleted with removePixelsMovement(). The
  // general update signals a scroll update in the view which will ask
  // for the status bar content again (Editor::notifyScrollChanged).
  //
  // We weren't able to reproduce this scenario anyway (which should
  // be visible with the ASSERT() above).
  if (!m_pixelsMovement)
    return false;

  const Transformation& transform(getTransformation(editor));
  gfx::Size imageSize = m_pixelsMovement->getInitialImageSize();

  int w = int(transform.bounds().w);
  int h = int(transform.bounds().h);
  int gcd = base::gcd(w, h);
  StatusBar::instance()->setStatusText(
    100,
    fmt::format(
      ":pos: {} {}"
      " :size: {} {}"
      " :selsize: {} {} [{:.02f}% {:.02f}%]"
      " :angle: {:.1f}"
      " :aspect_ratio: {}:{}",
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
      h/gcd));

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

  MOVPIXS_TRACE("MOVPIXS: onBeforeCommandExecution %s\n", command->id().c_str());

  // If the command is for other editor, we don't drop pixels.
  if (!isActiveEditor())
    return;

  if (layer_is_locked(m_editor) &&
      (command->id() == CommandId::Flip() ||
      command->id() == CommandId::Cut() ||
      command->id() == CommandId::Clear() ||
      command->id() == CommandId::Rotate())) {
    ev.cancel();
    return;
  }

  // We don't need to drop the pixels if a MoveMaskCommand of Content is executed.
  if (auto moveMaskCmd = dynamic_cast<MoveMaskCommand*>(command)) {
    if (moveMaskCmd->getTarget() == MoveMaskCommand::Content) {
      if (layer_is_locked(m_editor)) {
        ev.cancel();
        return;
      }
      gfx::Point delta = moveMaskCmd->getMoveThing().getDelta(UIContext::instance());
      // Verify Shift condition of the MoveMaskCommand (i.e. wrap = true)
      if (moveMaskCmd->isWrap()) {
        m_pixelsMovement->shift(delta.x, delta.y);
      }
      else {
        translate(gfx::PointF(delta));
      }
      // We've processed the selection content movement right here.
      ev.cancel();
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

      if (floatingImage->isTilemap()) {
        Site site = m_editor->getSite();
        ASSERT(site.tileset());
        Clipboard::instance()->
          copyTilemap(floatingImage.get(),
                      floatingMask.get(),
                      document->sprite()->palette(m_editor->frame()),
                      site.tileset());
      }
      else {
        Clipboard::instance()->
          copyImage(floatingImage.get(),
                    floatingMask.get(),
                    document->sprite()->palette(m_editor->frame()));
      }
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
    if (auto flipCommand = dynamic_cast<FlipCommand*>(command)) {
      this->flip(flipCommand->getFlipType());

      ev.cancel();
      return;
    }
  }
  // Rotate is quite simple, we can add the angle to the current transformation.
  else if (command->id() == CommandId::Rotate()) {
    if (auto rotate = dynamic_cast<RotateCommand*>(command)) {
      if (rotate->flipMask()) {
        this->rotate(rotate->angle());

        ev.cancel();
        return;
      }
    }
  }
  // We can use previous/next frames while transforming the selection
  // to switch between frames
  else if (command->id() == CommandId::GotoPreviousFrame() ||
           command->id() == CommandId::GotoPreviousFrameWithSameTag()) {
    if (m_pixelsMovement->gotoFrame(-1)) {
      ev.cancel();
      return;
    }
  }
  else if (command->id() == CommandId::GotoNextFrame() ||
           command->id() == CommandId::GotoNextFrameWithSameTag()) {
    if (m_pixelsMovement->gotoFrame(+1)) {
      ev.cancel();
      return;
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

  if (m_pixelsMovement &&
      !m_pixelsMovement->canHandleFrameChange()) {
    dropPixels();
  }
}

void MovingPixelsState::onBeforeLayerChanged(Editor* editor)
{
  if (!isActiveDocument())
    return;

  if (m_pixelsMovement)
    dropPixels();
}

void MovingPixelsState::onBeforeRangeChanged(Timeline* timeline)
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

void MovingPixelsState::onRenderTimer()
{
  m_pixelsMovement->setFastMode(false);
  m_renderTimer.stop();
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

void MovingPixelsState::onPivotChange()
{
  ContextBar* contextBar = App::instance()->contextBar();
  contextBar->updateForMovingPixels(getTransformation(m_editor));
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
  MOVPIXS_TRACE("MOVPIXS: dropPixels\n");

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
  // Avoid receiving a onCommitMouseMove() message when
  // m_pixelsMovement is already nullptr.
  m_delayedMouseMove.stopTimer();

  m_pixelsMovement.reset();
  m_ctxConn.disconnect();
  m_opaqueConn.disconnect();
  m_transparentConn.disconnect();
}

KeyAction MovingPixelsState::getCurrentKeyAction() const
{
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
    case RotateNEHandle:
    case RotateSWHandle:
    case RotateSEHandle:
      keyContext = KeyContext::RotatingSelection;
      break;
    case SkewNHandle:
    case SkewWHandle:
    case SkewEHandle:
    case SkewSHandle:
      keyContext = KeyContext::ScalingSelection;
      break;
  }

  // Ask to the editor the customization delegate for the active key
  // action/modifier for the pixels movement (snap to grid, angle
  // snap, etc.).
  if (auto customizable = m_editor->getCustomizationDelegate())
    return customizable->getPressedKeyAction(keyContext);
  else
    return KeyAction::None;
}

} // namespace app
