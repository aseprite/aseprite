// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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
#include "app/commands/params.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/clamp.h"
#include "base/string.h"
#include "doc/brush.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace app {

using namespace ui;

bool StateWithWheelBehavior::onMouseWheel(Editor* editor, MouseMessage* msg)
{
  gfx::Point delta = msg->wheelDelta();
  double dz = delta.x + delta.y;
  WheelAction wheelAction = WheelAction::None;
  bool scrollBigSteps = false;

  if (KeyboardShortcuts::instance()->hasMouseWheelCustomization()) {
    if (!Preferences::instance().editor.zoomWithSlide() && msg->preciseWheel())
      wheelAction = WheelAction::VScroll;
    else
      wheelAction = KeyboardShortcuts::instance()
        ->getWheelActionFromMouseMessage(KeyContext::MouseWheel, msg);
  }
  // Default behavior
  // TODO replace this code using KeyboardShortcuts::getDefaultMouseWheelTable()
  else {
    // Alt+mouse wheel changes the fg/bg colors
    if (msg->altPressed()) {
      if (msg->shiftPressed())
        wheelAction = WheelAction::BgColor;
      else
        wheelAction = WheelAction::FgColor;
    }
    // Normal behavior: mouse wheel zooms If the message is from a
    // precise wheel i.e. a trackpad/touch-like device, we scroll by
    // default.
    else if (Preferences::instance().editor.zoomWithWheel() && !msg->preciseWheel()) {
      if (msg->ctrlPressed() && msg->shiftPressed())
        wheelAction = WheelAction::Frame;
      else if (msg->ctrlPressed())
        wheelAction = WheelAction::BrushSize;
      else if (delta.x != 0 || msg->shiftPressed())
        wheelAction = WheelAction::HScroll;
      else
        wheelAction = WheelAction::Zoom;
    }
    // Zoom sliding two fingers
    else if (Preferences::instance().editor.zoomWithSlide() && msg->preciseWheel()) {
      if (msg->ctrlPressed() && msg->shiftPressed())
        wheelAction = WheelAction::Frame;
      else if (msg->ctrlPressed())
        wheelAction = WheelAction::BrushSize;
      else if (std::abs(delta.x) > std::abs(delta.y)) {
        delta.y = 0;
        dz = delta.x;
        wheelAction = WheelAction::HScroll;
      }
      else if (msg->shiftPressed()) {
        delta.x = 0;
        dz = delta.y;
        wheelAction = WheelAction::VScroll;
      }
      else {
        delta.x = 0;
        dz = delta.y;
        wheelAction = WheelAction::Zoom;
      }
    }
    // For laptops, it's convenient to that Ctrl+wheel zoom (because
    // it's the "pinch" gesture).
    else {
      if (msg->ctrlPressed())
        wheelAction = WheelAction::Zoom;
      else if (delta.x != 0 || msg->shiftPressed())
        wheelAction = WheelAction::HScroll;
      else
        wheelAction = WheelAction::VScroll;
    }
  }

  switch (wheelAction) {

    case WheelAction::None:
      // Do nothing
      break;

    case WheelAction::FgColor: {
      int lastIndex = get_current_palette()->size()-1;
      int newIndex = ColorBar::instance()->getFgColor().getIndex() + int(dz);
      newIndex = base::clamp(newIndex, 0, lastIndex);
      ColorBar::instance()->setFgColor(app::Color::fromIndex(newIndex));
      break;
    }

    case WheelAction::BgColor: {
      int lastIndex = get_current_palette()->size()-1;
      int newIndex = ColorBar::instance()->getBgColor().getIndex() + int(dz);
      newIndex = base::clamp(newIndex, 0, lastIndex);
      ColorBar::instance()->setBgColor(app::Color::fromIndex(newIndex));
      break;
    }

    case WheelAction::Frame: {
      Command* command = nullptr;

      if (dz < 0.0)
        command = Commands::instance()->byId(CommandId::GotoNextFrame());
      else if (dz > 0.0)
        command = Commands::instance()->byId(CommandId::GotoPreviousFrame());

      if (command)
        UIContext::instance()->executeCommand(command);
      break;
    }

    case WheelAction::Zoom: {
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

    case WheelAction::HScroll:
    case WheelAction::VScroll: {
      View* view = View::getView(editor);
      gfx::Point scroll = view->viewScroll();

      if (!msg->preciseWheel()) {
        gfx::Rect vp = view->viewportBounds();

        if (wheelAction == WheelAction::HScroll) {
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

    case WheelAction::BrushSize: {
      tools::Tool* tool = getActiveTool();
      ToolPreferences::Brush& brush =
        Preferences::instance().tool(tool).brush;

      brush.size(
        base::clamp(
          int(brush.size()+dz),
          // If we use the "static const int" member directly here,
          // we'll get a linker error (when compiling without
          // optimizations) because we should need to define the
          // address of these constants in "doc/brush.cpp"
          int(doc::Brush::kMinBrushSize),
          int(doc::Brush::kMaxBrushSize)));
      break;
    }

    case WheelAction::BrushAngle: {
      tools::Tool* tool = getActiveTool();
      ToolPreferences::Brush& brush =
        Preferences::instance().tool(tool).brush;

      int angle = brush.angle()+dz;
      while (angle < 0)
        angle += 180;
      angle %= 181;

      brush.angle(base::clamp(angle, 0, 180));
      break;
    }

    case WheelAction::ToolSameGroup: {
      tools::Tool* tool = getActiveTool();

      auto toolBox = App::instance()->toolBox();
      std::vector<tools::Tool*> tools;
      for (tools::Tool* t : *toolBox) {
        if (tool->getGroup() == t->getGroup())
          tools.push_back(t);
      }

      auto begin = tools.begin();
      auto end = tools.end();
      auto it = std::find(begin, end, tool);
      if (it != end) {
        if (dz < 0) {
          if (it == begin)
            it = end;
          --it;
        }
        else {
          ++it;
          if (it == end)
            it = begin;
        }
        if (tool != *it)
          ToolBar::instance()->selectTool(*it);
      }
      break;
    }

    case WheelAction::ToolOtherGroup: {
      tools::Tool* tool = getActiveTool();
      auto toolBox = App::instance()->toolBox();
      auto begin = toolBox->begin_group();
      auto end = toolBox->end_group();
      auto it = std::find(begin, end, tool->getGroup());
      if (it != end) {
        if (dz < 0) {
          if (it == begin)
            it = end;
          --it;
        }
        else {
          ++it;
          if (it == end)
            it = begin;
        }
        ToolBar::instance()->selectToolGroup(*it);
      }
      break;
    }

    case WheelAction::Layer: {
      Command* command = nullptr;
      if (dz < 0.0)
        command = Commands::instance()->byId(CommandId::GotoNextLayer());
      else if (dz > 0.0)
        command = Commands::instance()->byId(CommandId::GotoPreviousLayer());
      if (command)
        UIContext::instance()->executeCommand(command);
      break;
    }

    case WheelAction::InkOpacity: {
      tools::Tool* tool = getActiveTool();
      auto& toolPref = Preferences::instance().tool(tool);
      int opacity = toolPref.opacity();
      opacity = base::clamp(int(opacity+dz*255/10), 0, 255);
      toolPref.opacity(opacity);
      break;
    }

    case WheelAction::LayerOpacity: {
      Site site = UIContext::instance()->activeSite();
      if (site.layer() &&
          site.layer()->isImage() &&
          site.layer()->isEditable()) {
        Command* command = Commands::instance()->byId(CommandId::LayerOpacity());
        if (command) {
          int opacity = static_cast<doc::LayerImage*>(site.layer())->opacity();
          opacity = base::clamp(int(opacity+dz*255/10), 0, 255);

          Params params;
          params.set("opacity",
                     base::convert_to<std::string>(opacity).c_str());
          UIContext::instance()->executeCommand(command, params);
        }
      }
      break;
    }

    case WheelAction::CelOpacity: {
      Site site = UIContext::instance()->activeSite();
      if (site.layer() &&
          site.layer()->isImage() &&
          site.layer()->isEditable() &&
          site.cel()) {
        Command* command = Commands::instance()->byId(CommandId::CelOpacity());
        if (command) {
          int opacity = site.cel()->opacity();
          opacity = base::clamp(int(opacity+dz*255/10), 0, 255);
          Params params;
          params.set("opacity",
                     base::convert_to<std::string>(opacity).c_str());
          UIContext::instance()->executeCommand(command, params);
        }
      }
      break;
    }

    case WheelAction::Alpha: {
      disableQuickTool();

      ColorBar* colorBar = ColorBar::instance();
      Color c = colorBar->getFgColor();
      int a = c.getAlpha();
      a = base::clamp(int(a+dz*255/10), 0, 255);
      c.setAlpha(a);
      colorBar->setFgColor(c);
      break;
    }

    case WheelAction::HslHue:
    case WheelAction::HslSaturation:
    case WheelAction::HslLightness: {
      disableQuickTool();

      ColorBar* colorBar = ColorBar::instance();
      Color c = colorBar->getFgColor();
      double
        h = c.getHslHue(),
        s = c.getHslSaturation(),
        l = c.getHslLightness();
      switch (wheelAction) {
        case WheelAction::HslHue:        h = h+dz*10.0; break;
        case WheelAction::HslSaturation: s = s+dz/10.0; break;
        case WheelAction::HslLightness:  l = l+dz/10.0; break;
      }
      colorBar->setFgColor(Color::fromHsl(base::clamp(h, 0.0, 360.0),
                                          base::clamp(s, 0.0, 1.0),
                                          base::clamp(l, 0.0, 1.0)));
      break;
    }

    case WheelAction::HsvHue:
    case WheelAction::HsvSaturation:
    case WheelAction::HsvValue: {
      disableQuickTool();

      ColorBar* colorBar = ColorBar::instance();
      Color c = colorBar->getFgColor();
      double
        h = c.getHsvHue(),
        s = c.getHsvSaturation(),
        v = c.getHsvValue();
      switch (wheelAction) {
        case WheelAction::HsvHue:        h = h+dz*10.0; break;
        case WheelAction::HsvSaturation: s = s+dz/10.0; break;
        case WheelAction::HsvValue:      v = v+dz/10.0; break;
      }
      colorBar->setFgColor(Color::fromHsv(base::clamp(h, 0.0, 360.0),
                                          base::clamp(s, 0.0, 1.0),
                                          base::clamp(v, 0.0, 1.0)));
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

tools::Tool* StateWithWheelBehavior::getActiveTool()
{
  disableQuickTool();
  return App::instance()->activeToolManager()->activeTool();
}

void StateWithWheelBehavior::disableQuickTool()
{
  auto atm = App::instance()->activeToolManager();
  if (atm->quickTool()) {
    // As Ctrl key could active the Move tool, and Ctrl+mouse wheel can
    // change the size of the tool, we want to remove the quick tool so
    // the effect is for the selected tool.
    atm->newQuickToolSelectedFromEditor(nullptr);
  }
}

} // namespace app
