// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  int dz = msg->wheelDelta().x + msg->wheelDelta().y;
  WHEEL_ACTION wheelAction = WHEEL_NONE;
  bool scrollBigSteps = false;

  // Alt+mouse wheel changes the fg/bg colors
  if (msg->altPressed()) {
    if (msg->shiftPressed())
      wheelAction = WHEEL_BG;
    else
      wheelAction = WHEEL_FG;
  }
  // Normal behavior: mouse wheel zooms
  else if (Preferences::instance().editor.zoomWithWheel()) {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_FRAME;
    else if (msg->wheelDelta().x != 0 || msg->shiftPressed())
      wheelAction = WHEEL_HSCROLL;
    else
      wheelAction = WHEEL_ZOOM;
  }
  // For laptops, it's convenient to that Ctrl+wheel zoom (because
  // it's the "pinch" gesture).
  else {
    if (msg->ctrlPressed())
      wheelAction = WHEEL_ZOOM;
    else if (msg->wheelDelta().x != 0 || msg->shiftPressed())
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
        int newIndex = 0;
        if (ColorBar::instance()->getFgColor().getType() == app::Color::IndexType) {
          int lastIndex = get_current_palette()->size()-1;

          newIndex = ColorBar::instance()->getFgColor().getIndex() + dz;
          newIndex = MID(0, newIndex, lastIndex);
        }
        ColorBar::instance()->setFgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_BG:
      {
        int newIndex = 0;
        if (ColorBar::instance()->getBgColor().getType() == app::Color::IndexType) {
          int lastIndex = get_current_palette()->size()-1;

          newIndex = ColorBar::instance()->getBgColor().getIndex() + dz;
          newIndex = MID(0, newIndex, lastIndex);
        }
        ColorBar::instance()->setBgColor(app::Color::fromIndex(newIndex));
      }
      break;

    case WHEEL_FRAME:
      {
        Command* command = CommandsModule::instance()->getCommandByName
          ((dz < 0) ? CommandId::GotoNextFrame:
                      CommandId::GotoPreviousFrame);
        if (command)
          UIContext::instance()->executeCommand(command);
      }
      break;

    case WHEEL_ZOOM: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      render::Zoom zoom = editor->zoom();
      if (dz < 0) {
        while (dz++ < 0)
          zoom.in();
      }
      else {
        while (dz-- > 0)
          zoom.out();
      }

      if (editor->zoom() != zoom) {
        bool center = Preferences::instance().editor.zoomFromCenterWithWheel();

        editor->setZoomAndCenterInMouse(
          zoom, mouseMsg->position(),
          (center ? Editor::ZoomBehavior::CENTER:
                    Editor::ZoomBehavior::MOUSE));
      }
      break;
    }

    case WHEEL_HSCROLL:
    case WHEEL_VSCROLL: {
      View* view = View::getView(editor);
      gfx::Point scroll = view->viewScroll();
      gfx::Point delta(0, 0);

      if (msg->preciseWheel()) {
        delta = msg->wheelDelta();
      }
      else {
        gfx::Rect vp = view->viewportBounds();

        if (wheelAction == WHEEL_HSCROLL) {
          delta.x = dz * vp.w;
        }
        else {
          delta.y = dz * vp.h;
        }

        if (scrollBigSteps) {
          delta /= 2;
        }
        else {
          delta /= 10;
        }
      }

      editor->setEditorScroll(scroll+delta, true);
      break;
    }

  }

  return true;
}

} // namespace app
