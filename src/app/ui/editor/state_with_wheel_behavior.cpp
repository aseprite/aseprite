// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/state_with_wheel_behavior.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/palette.h"
#include "ui/message.h"
#include "ui/theme.h"

namespace app {

using namespace ui;

enum WHEEL_ACTION { WHEEL_NONE,
                    WHEEL_ZOOM,
                    WHEEL_VSCROLL,
                    WHEEL_HSCROLL,
                    WHEEL_FG,
                    WHEEL_BG,
                    WHEEL_FRAME };

bool StateWithWheelBehavior::onMouseWheel(Editor* editor, MouseMessage* msg)
{
  gfx::Point delta = msg->wheelDelta();
  double dz = delta.x + delta.y;
  WHEEL_ACTION wheelAction = WHEEL_NONE;
  bool scrollBigSteps = false;

  // Alt+mouse wheel changes the fg/bg colors
  if (msg->altPressed()) {
    if (msg->shiftPressed())
      wheelAction = WHEEL_BG;
    else
      wheelAction = WHEEL_FG;
  }
  // Normal behavior: mouse wheel zooms If the message is from a
  // precise wheel i.e. a trackpad/touch-like device, we scroll by
  // default.
  else if (Preferences::instance().editor.zoomWithWheel() && !msg->preciseWheel()) {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_FRAME;
    else if (delta.x != 0 || msg->shiftPressed())
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_ZOOM;
  }
  // Zoom sliding two fingers
  else if (Preferences::instance().editor.zoomWithSlide() && msg->preciseWheel()) {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_FRAME;
    else if (std::abs(delta.x) > std::abs(delta.y)) {
      delta.y = 0;
      dz = delta.x;
      wheelAction = WHEEL_HSCROLL;
    }
    else if (msg->shiftPressed()) {
      delta.x = 0;
      dz = delta.y;
      wheelAction = WHEEL_VSCROLL;
    }
    else {
      delta.x = 0;
      dz = delta.y;
      wheelAction = WHEEL_ZOOM;
    }
  }
  // For laptops, it's convenient to that Ctrl+wheel zoom (because
  // it's the "pinch" gesture).
  else {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_ZOOM;
    else if (delta.x != 0 || msg->shiftPressed())
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_VSCROLL;
  }

  switch (wheelAction) {

    case WHEEL_NONE:
      // Do nothing
      break;

    case WHEEL_FG:
      {
        int lastIndex = get_current_palette()->size()-1;
        int newIndex = ColorBar::instance()->getFgColor().getIndex() + int(dz);
        newIndex = MID(0, newIndex, lastIndex);
        ColorBar::instance()->setFgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_BG:
      {
        int lastIndex = get_current_palette()->size()-1;
        int newIndex = ColorBar::instance()->getBgColor().getIndex() + int(dz);
        newIndex = MID(0, newIndex, lastIndex);
        ColorBar::instance()->setBgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_FRAME: {
      Command* command = nullptr;

      if (dz < 0.0)
        command = Commands::instance()->byId(CommandId::GotoNextFrame);
      else if (dz > 0.0)
        command = Commands::instance()->byId(CommandId::GotoPreviousFrame);

      if (command)
        UIContext::instance()->executeCommand(command);
      break;
    }

    case WHEEL_ZOOM: {
      render::Zoom zoom = editor->zoom();

      if (msg->preciseWheel()) {
        dz /= 1.5;
        if (dz < -1.0) dz = -1.0;
        else if (dz > 1.0) dz = 1.0;
      }

      zoom = render::Zoom::fromLinearScale(zoom.linearScale() - int(dz));

      setZoom(editor, zoom, msg->position());
      break;
    }

    case WHEEL_HSCROLL:
    case WHEEL_VSCROLL: {
      View* view = View::getView(editor);
      gfx::Point scroll = view->viewScroll();

      if (!msg->preciseWheel()) {
        gfx::Rect vp = view->viewportBounds();

        if (wheelAction == WHEEL_HSCROLL) {
          delta.x = int(dz * vp.w);
        }
        else {
          delta.y = int(dz * vp.h);
        }

        if (scrollBigSteps) {
          delta /= 2;
        }
        else {
          delta /= 10;
        }
      }

      editor->setEditorScroll(scroll+delta);
      break;
    }

  }

  return true;
}

bool StateWithWheelBehavior::onTouchMagnify(Editor* editor, ui::TouchMessage* msg)
{
  render::Zoom zoom = editor->zoom();
  zoom = render::Zoom::fromScale(
    zoom.internalScale() + zoom.internalScale() * msg->magnification());

  setZoom(editor, zoom, msg->position());
  return true;
}

void StateWithWheelBehavior::setZoom(Editor* editor,
                                     const render::Zoom& zoom,
                                     const gfx::Point& mousePos)
{
  bool center = Preferences::instance().editor.zoomFromCenterWithWheel();

  editor->setZoomAndCenterInMouse(
    zoom, mousePos,
    (center ? Editor::ZoomBehavior::CENTER:
              Editor::ZoomBehavior::MOUSE));
}

} // namespace app
