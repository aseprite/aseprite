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

#include "widgets/editor/moving_pixels_state.h"

#include "app.h"
#include "app/color_utils.h"
#include "base/unique_ptr.h"
#include "commands/cmd_flip.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "gfx/rect.h"
#include "gui/manager.h"
#include "gui/message.h"
#include "gui/system.h"
#include "gui/view.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/algorithm/flip_image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/clipboard.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_customization_delegate.h"
#include "widgets/editor/pixels_movement.h"
#include "widgets/editor/standby_state.h"
#include "widgets/editor/transform_handles.h"
#include "widgets/statebar.h"

#include <allegro.h>

MovingPixelsState::MovingPixelsState(Editor* editor, Message* msg, PixelsMovement* pixelsMovement, HandleType handle)
  : m_currentEditor(editor)
  , m_discarded(false)
{
  // MovingPixelsState needs a selection tool to avoid problems
  // sharing the extra cel between the drawing cursor preview and the
  // pixels movement/transformation preview.
  //ASSERT(!editor->getCurrentEditorTool()->getInk(0)->isSelection());

  EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
  m_pixelsMovement = pixelsMovement;

  if (handle != NoHandle) {
    int u, v;
    editor->screenToEditor(msg->mouse.x, msg->mouse.y, &u, &v);
    m_pixelsMovement->catchImage(u, v, handle);

    editor->captureMouse();
  }

  // Setup mask color
  setTransparentColor(app_get_statusbar()->getTransparentColor());

  // Add this class as:
  // - listener of the UI context: so we know if the user wants to execute
  //   other command, so we can drop pixels.
  // - listener of the status bar to know if the user has changed the
  //   transparent color.
  UIContext::instance()->addListener(this);
  app_get_statusbar()->addListener(this);

  // Show controls to modify the "pixels movement" options (e.g. the
  // transparent color).
  app_get_statusbar()->showMovePixelsOptions();

  // Add the current editor as filter for key message of the manager
  // so we can catch the Enter key, and avoid to execute the
  // PlayAnimation command.
  m_currentEditor->getManager()->addMessageFilter(JM_KEYPRESSED, m_currentEditor);
  m_currentEditor->getManager()->addMessageFilter(JM_KEYRELEASED, m_currentEditor);
}

MovingPixelsState::~MovingPixelsState()
{
  UIContext::instance()->removeListener(this);
  app_get_statusbar()->removeListener(this);

  delete m_pixelsMovement;

  m_currentEditor->getManager()->removeMessageFilter(JM_KEYPRESSED, m_currentEditor);
  m_currentEditor->getManager()->removeMessageFilter(JM_KEYRELEASED, m_currentEditor);
}

EditorState::BeforeChangeAction MovingPixelsState::onBeforeChangeState(Editor* editor, EditorState* newState)
{
  ASSERT(m_pixelsMovement != NULL);

  // If we are changing to another state, we've to drop the image.
  if (m_pixelsMovement->isDragging())
    m_pixelsMovement->dropImageTemporarily();

  // Drop pixels if we are changing to a non-temporary state (a
  // temporary state is something like ScrollingState).
  if (!newState || !newState->isTemporalState()) {
    if (!m_discarded)
      m_pixelsMovement->dropImage();

    editor->getDocument()->resetTransformation();

    delete m_pixelsMovement;
    m_pixelsMovement = NULL;

    editor->releaseMouse();

    app_get_statusbar()->hideMovePixelsOptions();
    return DiscardState;
  }
  else {
    editor->releaseMouse();
    return KeepState;
  }
}

void MovingPixelsState::onCurrentToolChange(Editor* editor)
{
  ASSERT(m_pixelsMovement != NULL);

  tools::Tool* current_tool = editor->getCurrentEditorTool();

  // If the user changed the tool when he/she is moving pixels,
  // we have to drop the pixels only if the new tool is not selection...
  if (m_pixelsMovement &&
      (!current_tool->getInk(0)->isSelection() ||
       !current_tool->getInk(1)->isSelection())) {
    // We have to drop pixels
    dropPixels(editor);
  }
}

bool MovingPixelsState::onMouseDown(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  Decorator* decorator = static_cast<Decorator*>(editor->getDecorator());
  Document* document = editor->getDocument();

  // Start scroll loop
  if (checkForScroll(editor, msg))
    return true;

  // Transform selected pixels
  if (document->isMaskVisible() &&
      decorator->getTransformHandles(editor)) {
    TransformHandles* transfHandles = decorator->getTransformHandles(editor);

    // Get the handle covered by the mouse.
    HandleType handle = transfHandles->getHandleAtPoint(editor,
                                                        gfx::Point(msg->mouse.x, msg->mouse.y),
                                                        getTransformation(editor));

    if (handle != NoHandle) {
      // Re-catch the image
      int x, y;
      editor->screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);
      m_pixelsMovement->catchImageAgain(x, y, handle);

      editor->captureMouse();
      return true;
    }
  }

  // Start "moving pixels" loop
  if (editor->isInsideSelection() && (msg->mouse.left ||
                                      msg->mouse.right)) {
    // In case that the user is pressing the copy-selection keyboard shortcut.
    EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();
    if (customization && customization->isCopySelectionKeyPressed()) {
      // Stamp the pixels to create the copy.
      m_pixelsMovement->stampImage();
    }

    // Re-catch the image
    int x, y;
    editor->screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);
    m_pixelsMovement->catchImageAgain(x, y, MoveHandle);

    editor->captureMouse();
    return true;
  }
  // End "moving pixels" loop
  else {
    // Drop pixels (e.g. to start drawing)
    dropPixels(editor);
  }

  // Use StandbyState implementation
  return StandbyState::onMouseDown(editor, msg);
}

bool MovingPixelsState::onMouseUp(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  // Drop the image temporarily in this location (where the user releases the mouse)
  m_pixelsMovement->dropImageTemporarily();

  // Redraw the new pivot location.
  editor->invalidate();

  editor->releaseMouse();
  return true;
}

bool MovingPixelsState::onMouseMove(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  // If there is a button pressed
  if (m_pixelsMovement->isDragging()) {
    // Infinite scroll
    editor->controlInfiniteScroll(msg);

    // Get the position of the mouse in the sprite
    int x, y;
    editor->screenToEditor(msg->mouse.x, msg->mouse.y, &x, &y);

    // Get the customization for the pixels movement (snap to grid, angle snap, etc.).
    PixelsMovement::MoveModifier moveModifier = PixelsMovement::NormalMovement;

    if (editor->getCustomizationDelegate()->isSnapToGridKeyPressed())
      moveModifier |= PixelsMovement::SnapToGridMovement;

    if (editor->getCustomizationDelegate()->isAngleSnapKeyPressed())
      moveModifier |= PixelsMovement::AngleSnapMovement;

    if (editor->getCustomizationDelegate()->isMaintainAspectRatioKeyPressed())
      moveModifier |= PixelsMovement::MaintainAspectRatioMovement;

    // Invalidate handles area.
    Decorator* decorator = static_cast<Decorator*>(editor->getDecorator());
    TransformHandles* transfHandles = decorator->getTransformHandles(editor);
    transfHandles->invalidateHandles(editor, m_pixelsMovement->getTransformation());

    // Drag the image to that position
    gfx::Rect bounds = m_pixelsMovement->moveImage(x, y, moveModifier);

    // If "bounds" is empty is because the cel was not moved
    if (!bounds.isEmpty()) {
      // Redraw the extra cel in the new position
      jmouse_hide();
      editors_draw_sprite(editor->getSprite(),
                          bounds.x, bounds.y,
                          bounds.x+bounds.w-1,
                          bounds.y+bounds.h-1);
      jmouse_show();
    }
    editor->updateStatusBar();
    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingPixelsState::onMouseWheel(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  // Use StandbyState implementation
  return StandbyState::onMouseWheel(editor, msg);
}

bool MovingPixelsState::onSetCursor(Editor* editor)
{
  ASSERT(m_pixelsMovement != NULL);

  // Move selection
  if (m_pixelsMovement->isDragging()) {
    editor->hideDrawingCursor();
    jmouse_set_cursor(JI_CURSOR_MOVE);
    return true;
  }

  // Use StandbyState implementation
  return StandbyState::onSetCursor(editor);
}

bool MovingPixelsState::onKeyDown(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  if (msg->key.scancode == KEY_ENTER || // TODO make this key customizable
      msg->key.scancode == KEY_ENTER_PAD ||
      msg->key.scancode == KEY_ESC ) {
    dropPixels(editor);

    // The escape key drop pixels and deselect the mask.
    if (msg->key.scancode == KEY_ESC) { // TODO make this key customizable
      Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
      UIContext::instance()->executeCommand(cmd);
    }

    return true;
  }
  else {
    Command* command = NULL;
    Params* params = NULL;
    if (get_command_from_key_message(msg, &command, &params)) {
      // Intercept the "Cut" or "Copy" command to handle them locally
      // with the current m_pixelsMovement data.
      if (strcmp(command->short_name(), CommandId::Cut) == 0 ||
          strcmp(command->short_name(), CommandId::Copy) == 0) {
        // Copy the floating image to the clipboard.
        {
          Document* document = editor->getDocument();
          gfx::Point origin;
          UniquePtr<Image> floatingImage(m_pixelsMovement->getDraggedImageCopy(origin));
          clipboard::copy_image(floatingImage.get(),
                                document->getSprite()->getCurrentPalette(),
                                origin);
        }

        // In case of "Cut" command.
        if (strcmp(command->short_name(), CommandId::Cut) == 0) {
          // Discard the dragged image.
          m_pixelsMovement->discardImage();
          m_discarded = true;

          // Quit from MovingPixelsState, back to standby.
          editor->backToPreviousState();
        }

        // Return true because we've used the keyboard shortcut.
        return true;
      }
      // Flip Horizontally/Vertically commands are handled manually to
      // avoid dropping the floating region of pixels.
      else if (strcmp(command->short_name(), CommandId::Flip) == 0) {
        if (FlipCommand* flipCommand = dynamic_cast<FlipCommand*>(command)) {
          flipCommand->loadParams(params);
          m_pixelsMovement->flipImage(flipCommand->getFlipType());
          return true;
        }
      }
    }
  }

  // Use StandbyState implementation
  return StandbyState::onKeyDown(editor, msg);
}

bool MovingPixelsState::onKeyUp(Editor* editor, Message* msg)
{
  ASSERT(m_pixelsMovement != NULL);

  // Use StandbyState implementation
  return StandbyState::onKeyUp(editor, msg);
}

bool MovingPixelsState::onUpdateStatusBar(Editor* editor)
{
  ASSERT(m_pixelsMovement != NULL);

  const gfx::Transformation& transform(getTransformation(editor));
  Document* document = editor->getDocument();
  gfx::Size imageSize = m_pixelsMovement->getInitialImageSize();

  app_get_statusbar()->setStatusText
    (100, "Pos %d %d, Size %d %d, Orig: %3d %3d (%.02f%% %.02f%%), Angle %.1f",
     transform.bounds().x, transform.bounds().y,
     transform.bounds().w, transform.bounds().h,
     imageSize.w, imageSize.h,
     (double)transform.bounds().w*100.0/imageSize.w,
     (double)transform.bounds().h*100.0/imageSize.h,
     180.0 * transform.angle() / PI);

  return true;
}

// Before executing any command, we drop the pixels (go back to standby).
void MovingPixelsState::onCommandBeforeExecution(Context* context)
{
  if (m_pixelsMovement)
    dropPixels(m_currentEditor);
}

void MovingPixelsState::dispose()
{
  // Never called as MovingPixelsState is removed automatically as
  // StatusBar's listener.
}

void MovingPixelsState::onChangeTransparentColor(const Color& color)
{
  setTransparentColor(color);
}

void MovingPixelsState::setTransparentColor(const Color& color)
{
  ASSERT(current_editor != NULL);
  ASSERT(m_pixelsMovement != NULL);

  Sprite* sprite = current_editor->getSprite();
  ASSERT(sprite != NULL);

  PixelFormat format = sprite->getPixelFormat();
  m_pixelsMovement->setMaskColor(color_utils::color_for_image(color, format));
}

void MovingPixelsState::dropPixels(Editor* editor)
{
  // Just change to default state (StandbyState generally). We'll
  // receive an onBeforeChangeState() event after this call.
  editor->backToPreviousState();
}

gfx::Transformation MovingPixelsState::getTransformation(Editor* editor)
{
  return m_pixelsMovement->getTransformation();
}
