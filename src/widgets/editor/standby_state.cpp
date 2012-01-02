/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "widgets/editor/standby_state.h"

#include "app.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "gfx/rect.h"
#include "gui/alert.h"
#include "gui/message.h"
#include "gui/system.h"
#include "gui/view.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "widgets/color_bar.h"
#include "widgets/editor/drawing_state.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_customization_delegate.h"
#include "widgets/editor/handle_type.h"
#include "widgets/editor/moving_cel_state.h"
#include "widgets/editor/moving_pixels_state.h"
#include "widgets/editor/scrolling_state.h"
#include "widgets/editor/tool_loop_impl.h"
#include "widgets/editor/transform_handles.h"
#include "widgets/statebar.h"

#include <allegro.h>

enum WHEEL_ACTION { WHEEL_NONE,
		    WHEEL_ZOOM,
		    WHEEL_VSCROLL,
		    WHEEL_HSCROLL,
		    WHEEL_FG,
		    WHEEL_BG,
		    WHEEL_FRAME };

static int rotated_size_cursors[] = {
  JI_CURSOR_SIZE_R,
  JI_CURSOR_SIZE_TR,
  JI_CURSOR_SIZE_T,
  JI_CURSOR_SIZE_TL,
  JI_CURSOR_SIZE_L,
  JI_CURSOR_SIZE_BL,
  JI_CURSOR_SIZE_B,
  JI_CURSOR_SIZE_BR
};

static int rotated_rotate_cursors[] = {
  JI_CURSOR_ROTATE_R,
  JI_CURSOR_ROTATE_TR,
  JI_CURSOR_ROTATE_T,
  JI_CURSOR_ROTATE_TL,
  JI_CURSOR_ROTATE_L,
  JI_CURSOR_ROTATE_BL,
  JI_CURSOR_ROTATE_B,
  JI_CURSOR_ROTATE_BR
};

static inline bool has_shifts(Message* msg, int shift)
{
  return ((msg->any.shifts & shift) == shift);
}

StandbyState::StandbyState()
  : m_decorator(new Decorator(this))
{
}

StandbyState::~StandbyState()
{
  delete m_decorator;
}

void StandbyState::onAfterChangeState(Editor* editor)
{
  editor->setDecorator(m_decorator);
}

void StandbyState::onCurrentToolChange(Editor* editor)
{
  tools::Tool* currentTool = editor->getCurrentEditorTool();

  // If the user change from a selection tool to a non-selection tool,
  // or viceversa, we've to show or hide the transformation handles.
  // TODO Compare the ink (isSelection()) of the previous tool with
  // the new one.
  editor->invalidate();
}

bool StandbyState::onMouseDown(Editor* editor, Message* msg)
{
  if (editor->hasCapture())
    return true;

  UIContext* context = UIContext::instance();
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  tools::Ink* clicked_ink = current_tool->getInk(msg->mouse.right ? 1: 0);
  Sprite* sprite = editor->getSprite();

  // Each time an editor is clicked the current editor and the active
  // document are set.
  set_current_editor(editor);

  Document* document = editor->getDocument();
  context->setActiveDocument(document);

  // Start scroll loop
  if (msg->mouse.middle || clicked_ink->isScrollMovement()) {
    editor->setState(EditorStatePtr(new ScrollingState()));
    editor->captureMouse();
    return true;
  }

  // Move cel X,Y coordinates
  if (clicked_ink->isCelMovement()) {
    if ((sprite->getCurrentLayer()) &&
	(sprite->getCurrentLayer()->getType() == GFXOBJ_LAYER_IMAGE)) {
      // TODO you can move the `Background' with tiled mode
      if (sprite->getCurrentLayer()->is_background()) {
	Alert::show(PACKAGE
		    "<<You can't move the `Background' layer."
		    "||&Close");
      }
      else if (!sprite->getCurrentLayer()->is_moveable()) {
	Alert::show(PACKAGE "<<The layer movement is locked.||&Close");
      }
      else {
	// Change to MovingCelState
	editor->setState(EditorStatePtr(new MovingCelState(editor, msg)));
      }
    }
    return true;
  }

  // Transform selected pixels
  if (document->isMaskVisible() &&
      m_decorator->getTransformHandles(editor)) {
    TransformHandles* transfHandles = m_decorator->getTransformHandles(editor);

    // Get the handle covered by the mouse.
    HandleType handle = transfHandles->getHandleAtPoint(editor,
							gfx::Point(msg->mouse.x, msg->mouse.y),
							document->getTransformation());

    if (handle != NoHandle) {
      int x, y, opacity;
      Image* image = sprite->getCurrentImage(&x, &y, &opacity);
      if (image) {
	if (!sprite->getCurrentLayer()->is_writable()) {
	  Alert::show(PACKAGE "<<The layer is locked.||&Close");
	  return true;
	}

	// Change to MovingPixelsState
	editor->setState(EditorStatePtr(new MovingPixelsState(editor, msg, image, x, y, opacity, handle)));
      }
      return true;
    }
  }

  // Move selected pixels
  if (editor->isInsideSelection() &&
      current_tool->getInk(0)->isSelection() &&
      msg->mouse.left) {
    int x, y, opacity;
    Image* image = sprite->getCurrentImage(&x, &y, &opacity);
    if (image) {
      if (!sprite->getCurrentLayer()->is_writable()) {
	Alert::show(PACKAGE "<<The layer is locked.||&Close");
	return true;
      }

      // Change to MovingPixelsState
      editor->setState(EditorStatePtr(new MovingPixelsState(editor, msg, image, x, y, opacity, NoHandle)));
    }
    return true;
  }

  // Call the eyedropper command
  if (clicked_ink->isEyedropper()) {
    Command* eyedropper_cmd = 
      CommandsModule::instance()->getCommandByName(CommandId::Eyedropper);

    Params params;
    params.set("target", msg->mouse.right ? "background": "foreground");

    UIContext::instance()->executeCommand(eyedropper_cmd, &params);
    return true;
  }

  // Start the Tool-Loop
  if (sprite->getCurrentLayer()) {
    tools::ToolLoop* toolLoop = create_tool_loop(editor, context, msg);
    if (toolLoop)
      editor->setState(EditorStatePtr(new DrawingState(toolLoop, editor, msg)));
    return true;
  }

  return true;
}

bool StandbyState::onMouseUp(Editor* editor, Message* msg)
{
  editor->releaseMouse();
  return true;
}

bool StandbyState::onMouseMove(Editor* editor, Message* msg)
{
  editor->moveDrawingCursor();
  editor->updateStatusBar();
  return true;
}

bool StandbyState::onMouseWheel(Editor* editor, Message* msg)
{
  int dz = jmouse_z(1) - jmouse_z(0);
  WHEEL_ACTION wheelAction = WHEEL_NONE;
  bool scrollBigSteps = false;

  // Without modifiers
  if (!(msg->any.shifts & (KB_SHIFT_FLAG | KB_ALT_FLAG | KB_CTRL_FLAG))) {
    wheelAction = WHEEL_ZOOM;
  }
  else {
#if 1				// TODO make it configurable
    if (has_shifts(msg, KB_ALT_FLAG)) {
      if (has_shifts(msg, KB_SHIFT_FLAG))
	wheelAction = WHEEL_BG;
      else
	wheelAction = WHEEL_FG;
    }
    else if (has_shifts(msg, KB_CTRL_FLAG)) {
      wheelAction = WHEEL_FRAME;
    }
#else
    if (has_shifts(msg, KB_CTRL_FLAG))
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_VSCROLL;

    if (has_shifts(msg, KB_SHIFT_FLAG))
      scrollBigSteps = true;
#endif
  }

  switch (wheelAction) {

    case WHEEL_NONE:
      // Do nothing
      break;

    case WHEEL_FG:
      // if (m_state == EDITOR_STATE_STANDBY) 
      {
	int newIndex = 0;
	if (app_get_colorbar()->getFgColor().getType() == Color::IndexType) {
	  newIndex = app_get_colorbar()->getFgColor().getIndex() + dz;
	  newIndex = MID(0, newIndex, 255);
	}
	app_get_colorbar()->setFgColor(Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_BG:
      // if (m_state == EDITOR_STATE_STANDBY) 
      {
	int newIndex = 0;
	if (app_get_colorbar()->getBgColor().getType() == Color::IndexType) {
	  newIndex = app_get_colorbar()->getBgColor().getIndex() + dz;
	  newIndex = MID(0, newIndex, 255);
	}
	app_get_colorbar()->setBgColor(Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_FRAME:
      // if (m_state == EDITOR_STATE_STANDBY) 
      {
	Command* command = CommandsModule::instance()->getCommandByName
	  ((dz < 0) ? CommandId::GotoNextFrame:
		      CommandId::GotoPreviousFrame);
	if (command)
	  UIContext::instance()->executeCommand(command, NULL);
      }
      break;

    case WHEEL_ZOOM: {
      int zoom = MID(MIN_ZOOM, editor->getZoom()-dz, MAX_ZOOM);
      if (editor->getZoom() != zoom)
	editor->setZoomAndCenterInMouse(zoom, msg->mouse.x, msg->mouse.y);
      break;
    }

    case WHEEL_HSCROLL:
    case WHEEL_VSCROLL: {
      View* view = View::getView(editor);
      gfx::Rect vp = view->getViewportBounds();
      int dx = 0;
      int dy = 0;

      if (wheelAction == WHEEL_HSCROLL) {
	dx = dz * vp.w;
      }
      else {
	dy = dz * vp.h;
      }

      if (scrollBigSteps) {
	dx /= 2;
	dy /= 2;
      }
      else {
	dx /= 10;
	dy /= 10;
      }
		
      gfx::Point scroll = view->getViewScroll();

      editor->hideDrawingCursor();
      editor->setEditorScroll(scroll.x+dx, scroll.y+dy, true);
      editor->showDrawingCursor();
      break;
    }

  }

  return true;
}

bool StandbyState::onSetCursor(Editor* editor)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();

  if (current_tool) {
    tools::Ink* current_ink = current_tool->getInk(0);

    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (current_ink->isSelection()) {
      // See if the cursor is in some selection handle.
      if (m_decorator->onSetCursor(editor))
	return true;

      // Move pixels
      if (editor->isInsideSelection()) {
	EditorCustomizationDelegate* customization = editor->getCustomizationDelegate();

	editor->hideDrawingCursor();

	if (customization && customization->isCopySelectionKeyPressed())
	  jmouse_set_cursor(JI_CURSOR_NORMAL_ADD);
	else
	  jmouse_set_cursor(JI_CURSOR_MOVE);

	return true;
      }
    }
    else if (current_ink->isEyedropper()) {
      editor->hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
      return true;
    }
    else if (current_ink->isScrollMovement()) {
      editor->hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      return true;
    }
    else if (current_ink->isCelMovement()) {
      editor->hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_MOVE);
      return true;
    }
  }

  // Draw
  if (editor->canDraw()) {
    jmouse_set_cursor(JI_CURSOR_NULL);
    editor->showDrawingCursor();
  }
  // Forbidden
  else {
    editor->hideDrawingCursor();
    jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
  }

  return true;
}

bool StandbyState::onKeyDown(Editor* editor, Message* msg)
{
  return editor->processKeysToSetZoom(msg->key.scancode);
}

bool StandbyState::onKeyUp(Editor* editor, Message* msg)
{
  return false;
}

bool StandbyState::onUpdateStatusBar(Editor* editor)
{
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  const Sprite* sprite = editor->getSprite();
  int x, y;

  editor->screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);

  if (!sprite) {
    app_get_statusbar()->clearText();
  }
  // For eye-dropper
  else if (current_tool->getInk(0)->isEyedropper()) {
    int imgtype = sprite->getImgType();
    uint32_t pixel = sprite->getPixel(x, y);
    Color color = Color::fromImage(imgtype, pixel);

    int alpha = 255;
    switch (imgtype) {
      case IMAGE_RGB: alpha = _rgba_geta(pixel); break;
      case IMAGE_GRAYSCALE: alpha = _graya_geta(pixel); break;
    }

    char buf[256];
    usprintf(buf, "- Pos %d %d", x, y);

    app_get_statusbar()->showColor(0, buf, color, alpha);
  }
  else {
    Mask* mask = editor->getDocument()->getMask();

    app_get_statusbar()->setStatusText
      (0, "Pos %d %d, Size %d %d, Frame %d",
       x, y,
       ((mask && mask->bitmap)? mask->w: sprite->getWidth()),
       ((mask && mask->bitmap)? mask->h: sprite->getHeight()),
       sprite->getCurrentFrame()+1);
  }

  return true;
}

gfx::Transformation StandbyState::getTransformation(Editor* editor)
{
  return editor->getDocument()->getTransformation();
}

//////////////////////////////////////////////////////////////////////
// Decorator

StandbyState::Decorator::Decorator(StandbyState* standbyState)
  : m_transfHandles(NULL)
  , m_standbyState(standbyState)
{
}

StandbyState::Decorator::~Decorator()
{
  delete m_transfHandles;
}

TransformHandles* StandbyState::Decorator::getTransformHandles(Editor* editor)
{
  if (!m_transfHandles)
    m_transfHandles = new TransformHandles();

  return m_transfHandles;
}

bool StandbyState::Decorator::onSetCursor(Editor* editor)
{
  if (!editor->getDocument()->isMaskVisible())
    return false;

  const gfx::Transformation transformation(m_standbyState->getTransformation(editor));
  TransformHandles* tr = getTransformHandles(editor);
  HandleType handle = tr->getHandleAtPoint(editor,
					   gfx::Point(jmouse_x(0), jmouse_y(0)),
					   transformation);

  int newCursor = JI_CURSOR_NORMAL;

  switch (handle) {
    case ScaleNWHandle:		newCursor = JI_CURSOR_SIZE_TL; break;
    case ScaleNHandle:		newCursor = JI_CURSOR_SIZE_T; break;
    case ScaleNEHandle:		newCursor = JI_CURSOR_SIZE_TR; break;
    case ScaleWHandle:		newCursor = JI_CURSOR_SIZE_L; break;
    case ScaleEHandle:		newCursor = JI_CURSOR_SIZE_R; break;
    case ScaleSWHandle:		newCursor = JI_CURSOR_SIZE_BL; break;
    case ScaleSHandle:		newCursor = JI_CURSOR_SIZE_B; break;
    case ScaleSEHandle:		newCursor = JI_CURSOR_SIZE_BR; break;
    case RotateNWHandle:	newCursor = JI_CURSOR_ROTATE_TL; break;
    case RotateNHandle:		newCursor = JI_CURSOR_ROTATE_T; break;
    case RotateNEHandle:	newCursor = JI_CURSOR_ROTATE_TR; break;
    case RotateWHandle:		newCursor = JI_CURSOR_ROTATE_L; break;
    case RotateEHandle:		newCursor = JI_CURSOR_ROTATE_R; break;
    case RotateSWHandle:	newCursor = JI_CURSOR_ROTATE_BL; break;
    case RotateSHandle:		newCursor = JI_CURSOR_ROTATE_B; break;
    case RotateSEHandle:	newCursor = JI_CURSOR_ROTATE_BR; break;
    case PivotHandle:		newCursor = JI_CURSOR_HAND; break;
    default:
      return false;
  }

  // Adjust the cursor depending the current transformation angle.
  fixed angle = ftofix(128.0 * transformation.angle() / PI);
  angle = fixadd(angle, itofix(16));
  angle &= (255<<16);
  angle >>= 16;
  angle /= 32;

  if (newCursor >= JI_CURSOR_SIZE_TL && newCursor <= JI_CURSOR_SIZE_BR) {
    size_t num = sizeof(rotated_size_cursors) / sizeof(rotated_size_cursors[0]);
    size_t c;
    for (c=num-1; c>0; --c)
      if (rotated_size_cursors[c] == newCursor)
	break;

    newCursor = rotated_size_cursors[(c+angle) % num];
  }
  else if (newCursor >= JI_CURSOR_ROTATE_TL && newCursor <= JI_CURSOR_ROTATE_BR) {
    size_t num = sizeof(rotated_rotate_cursors) / sizeof(rotated_rotate_cursors[0]);
    size_t c;
    for (c=num-1; c>0; --c)
      if (rotated_rotate_cursors[c] == newCursor)
	break;

    newCursor = rotated_rotate_cursors[(c+angle) % num];
  }

  // Hide the drawing cursor (just in case) and show the new system cursor.
  editor->hideDrawingCursor();
  jmouse_set_cursor(newCursor);
  return true;
}

void StandbyState::Decorator::preRenderDecorator(EditorPreRender* render)
{
  // Do nothing
}

void StandbyState::Decorator::postRenderDecorator(EditorPostRender* render)
{
  Editor* editor = render->getEditor();

  // Draw transformation handles (if the mask is visible and isn't frozen).
  if (editor->getDocument()->isMaskVisible() &&
      !editor->getDocument()->getMask()->isFrozen()) {
    // And draw only when the user has a selection tool as active tool.
    tools::Tool* currentTool = editor->getCurrentEditorTool();

    if (currentTool->getInk(0)->isSelection())
      getTransformHandles(editor)->drawHandles(editor,
					       m_standbyState->getTransformation(editor));
  }
}
