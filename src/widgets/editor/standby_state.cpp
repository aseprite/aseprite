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
#include "core/cfg.h"
#include "gfx/rect.h"
#include "gui/alert.h"
#include "gui/message.h"
#include "gui/system.h"
#include "gui/view.h"
#include "modules/editors.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/misc.h"
#include "widgets/color_bar.h"
#include "widgets/editor/drawing_state.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/moving_pixels_state.h"
#include "widgets/editor/scrolling_state.h"
#include "widgets/editor/tool_loop_impl.h"
#include "widgets/statebar.h"

#include <allegro.h>

enum WHEEL_ACTION { WHEEL_NONE,
		    WHEEL_ZOOM,
		    WHEEL_VSCROLL,
		    WHEEL_HSCROLL,
		    WHEEL_FG,
		    WHEEL_BG,
		    WHEEL_FRAME };

static inline bool has_shifts(Message* msg, int shift)
{
  return ((msg->any.shifts & shift) == shift);
}

StandbyState::StandbyState()
{
}

StandbyState::~StandbyState()
{
}

bool StandbyState::onMouseDown(Editor* editor, Message* msg)
{
  if (editor->hasCapture())
    return true;

  UIContext* context = UIContext::instance();
  tools::Tool* current_tool = editor->getCurrentEditorTool();
  Sprite* sprite = editor->getSprite();

  // Each time an editor is clicked the current editor and the active
  // document are set.
  set_current_editor(editor);
  context->setActiveDocument(editor->getDocument());

  // Start scroll loop
  if (msg->mouse.middle ||
      current_tool->getInk(msg->mouse.right ? 1: 0)->isScrollMovement()) {
    editor->setState(EditorStatePtr(new ScrollingState()));
    editor->captureMouse();
    return true;
  }

  // Move frames position
  if (current_tool->getInk(msg->mouse.right ? 1: 0)->isCelMovement()) {
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
	bool click2 = get_config_bool("Options", "MoveClick2", FALSE);

	// TODO replace "interactive_move_layer" with a new EditorState
	interactive_move_layer(click2 ? Editor::MODE_CLICKANDCLICK:
					Editor::MODE_CLICKANDRELEASE,
			       TRUE, NULL);
      }
    }
  }
  // Move selected pixels
  else if (editor->isInsideSelection() &&
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
      editor->setState(EditorStatePtr(new MovingPixelsState(editor, msg, image, x, y, opacity)));
    }
  }
  // Call the eyedropper command
  else if (current_tool->getInk(msg->mouse.right ? 1: 0)->isEyedropper()) {
    Command* eyedropper_cmd = 
      CommandsModule::instance()->getCommandByName(CommandId::Eyedropper);

    Params params;
    params.set("target", msg->mouse.right ? "background": "foreground");

    UIContext::instance()->executeCommand(eyedropper_cmd, &params);
    return true;
  }
  // Start the Tool-Loop
  else if (sprite->getCurrentLayer()) {
    tools::ToolLoop* toolLoop = create_tool_loop(editor, context, msg);
    if (toolLoop)
      editor->setState(EditorStatePtr(new DrawingState(toolLoop, editor, msg)));
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
    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (current_tool->getInk(0)->isSelection()) {
      // Move pixels
      if (editor->isInsideSelection()) {
	editor->hideDrawingCursor();

	if (key[KEY_LCONTROL] ||
	    key[KEY_RCONTROL]) // TODO configurable keys
	  jmouse_set_cursor(JI_CURSOR_NORMAL_ADD);
	else
	  jmouse_set_cursor(JI_CURSOR_MOVE);

	return true;
      }
    }
    else if (current_tool->getInk(0)->isEyedropper()) {
      editor->hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
      return true;
    }
    else if (current_tool->getInk(0)->isScrollMovement()) {
      editor->hideDrawingCursor();
      jmouse_set_cursor(JI_CURSOR_SCROLL);
      return true;
    }
    else if (current_tool->getInk(0)->isCelMovement()) {
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
  Sprite* sprite = editor->getSprite();
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
