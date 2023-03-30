// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/string.h"
#include "doc/brush.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/theme.h"

#include "app/tools/ink.h"
#include "app/ui/skin/skin_theme.h"

namespace app {

using namespace ui;
using PreciseWheel = StateWithWheelBehavior::PreciseWheel;

template<typename T>
static inline void adjust_value(PreciseWheel preciseWheel, double dz, T& v, T min, T max)
{
  if (preciseWheel == PreciseWheel::On)
    v = std::clamp<T>(T(v+dz), min, max);
  else
    v = std::clamp<T>(T(v+dz*max/T(10)), min, max);
}

template<typename T>
static inline void adjust_hue(PreciseWheel preciseWheel, double dz, T& v, T min, T max)
{
  if (preciseWheel == PreciseWheel::On)
    v = std::clamp<T>(T(v+dz), min, max);
  else
    v = std::clamp<T>(T(v+dz*T(10)), min, max);
}

static inline void adjust_unit(PreciseWheel preciseWheel, double dz, double& v)
{
  v = std::clamp<double>(v+(preciseWheel == PreciseWheel::On ? dz/100.0: dz/25.0), 0.0, 1.0);
}

StateWithWheelBehavior::StateWithWheelBehavior()
{
}

bool StateWithWheelBehavior::onMouseWheel(Editor* editor, MouseMessage* msg)
{
  gfx::Point delta = msg->wheelDelta();
  double dz = delta.x + delta.y;
  WheelAction wheelAction = WheelAction::None;

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
      bool tilemapMode = (editor->getSite().tilemapMode() == TilemapMode::Tiles);
      if (msg->shiftPressed())
        wheelAction = (tilemapMode ? WheelAction::BgTile : WheelAction::BgColor);
      else
        wheelAction = (tilemapMode ? WheelAction::FgTile : WheelAction::FgColor);
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

  processWheelAction(editor,
                     wheelAction,
                     msg->position(),
                     delta,
                     dz,
                     // The possibility for big scroll steps was lost
                     // in history (it was possible using Shift key in
                     // very old versions, now Shift is used for
                     // horizontal scroll).
                     ScrollBigSteps::Off,
                     (msg->preciseWheel() ?
                      PreciseWheel::On:
                      PreciseWheel::Off),
                     FromMouseWheel::On);
  return true;
}

void StateWithWheelBehavior::processWheelAction(
  Editor* editor,
  WheelAction wheelAction,
  const gfx::Point& position,
  gfx::Point delta,
  double dz,
  const ScrollBigSteps scrollBigSteps,
  const PreciseWheel preciseWheel,
  const FromMouseWheel fromMouseWheel)
{
  switch (wheelAction) {

    case WheelAction::None:
      // Do nothing
      break;

    case WheelAction::FgColor: {
      int lastIndex = get_current_palette()->size()-1;
      int newIndex = initialFgColor().getIndex() + int(dz);
      newIndex = std::clamp(newIndex, 0, lastIndex);
      changeFgColor(app::Color::fromIndex(newIndex));
      break;
    }

    case WheelAction::BgColor: {
      int lastIndex = get_current_palette()->size()-1;
      int newIndex = initialBgColor().getIndex() + int(dz);
      newIndex = std::clamp(newIndex, 0, lastIndex);
      ColorBar::instance()->setBgColor(app::Color::fromIndex(newIndex));
      break;
    }

    case WheelAction::FgTile: {
      auto tilesView = ColorBar::instance()->getTilesView();
      if (tilesView->tileset()) {
        int lastIndex = tilesView->tileset()->size()-1;
        int newIndex = initialFgTileIndex() + int(dz);
        newIndex = std::clamp(newIndex, 0, lastIndex);
        ColorBar::instance()->setFgTile(newIndex);
      }
      break;
    }

    case WheelAction::BgTile: {
      auto tilesView = ColorBar::instance()->getTilesView();
      if (tilesView->tileset()) {
        int lastIndex = tilesView->tileset()->size()-1;
        int newIndex = initialBgTileIndex() + int(dz);
        newIndex = std::clamp(newIndex, 0, lastIndex);
        ColorBar::instance()->setBgTile(newIndex);
      }
      break;
    }

    case WheelAction::Frame: {
      frame_t deltaFrame = 0;
      if (preciseWheel == PreciseWheel::On) {
        if (dz < 0.0)
          deltaFrame = +1;
        else if (dz > 0.0)
          deltaFrame = -1;
      }
      else {
        deltaFrame = -dz;
      }

      frame_t frame = initialFrame(editor) + deltaFrame;
      frame_t nframes = editor->sprite()->totalFrames();
      while (frame < 0)
        frame += nframes;
      while (frame >= nframes)
        frame -= nframes;

      editor->setFrame(frame);
      break;
    }

    case WheelAction::Zoom: {
      render::Zoom zoom = initialZoom(editor);

      if (preciseWheel == PreciseWheel::On) {
        dz /= 1.5;
        if (dz < -1.0) dz = -1.0;
        else if (dz > 1.0) dz = 1.0;
      }

      zoom = render::Zoom::fromLinearScale(zoom.linearScale() - int(dz));

      setZoom(editor, zoom, position);
      break;
    }

    case WheelAction::HScroll:
    case WheelAction::VScroll: {
      View* view = View::getView(editor);
      gfx::Point scroll = initialScroll(editor);

      if (preciseWheel == PreciseWheel::Off) {
        gfx::Rect vp = view->viewportBounds();

        if (wheelAction == WheelAction::HScroll) {
          delta.x = int(dz * vp.w);
        }
        else {
          delta.y = int(dz * vp.h);
        }

        if (scrollBigSteps == ScrollBigSteps::On) {
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

      if (fromMouseWheel == FromMouseWheel::On) {
#if LAF_WINDOWS || LAF_LINUX
        // By default on macOS the mouse wheel is correct, up increase
        // brush size, and down decrease it. But on Windows and Linux
        // it's inverted.
        dz = -dz;
#endif

        // We can configure the mouse wheel for brush size to behave as
        // in previous versions.
        if (Preferences::instance().editor.invertBrushSizeWheel())
          dz = -dz;
      }

      brush.size(
        std::clamp(
          int(initialBrushSize()+dz),
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

      int angle = initialBrushAngle()+dz;
      while (angle < 0)
        angle += 180;
      angle %= 181;

      brush.angle(std::clamp(angle, 0, 180));
      break;
    }

    case WheelAction::ToolSameGroup: {
      const tools::Tool* tool = getInitialToolInActiveGroup();

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
        int i = std::round(dz);
        while (i < 0) {
          ++i;
          if (it == begin)
            it = end;
          --it;
        }
        while (i > 0) {
          --i;
          ++it;
          if (it == end)
            it = begin;
        }
        onToolChange(*it);
      }
      break;
    }

    case WheelAction::ToolOtherGroup: {
      const tools::Tool* tool = initialTool();

      auto toolBox = App::instance()->toolBox();
      auto begin = toolBox->begin_group();
      auto end = toolBox->end_group();
      auto it = std::find(begin, end, tool->getGroup());
      if (it != end) {
        int i = std::round(dz);
        while (i < 0) {
          ++i;
          if (it == begin)
            it = end;
          --it;
        }
        while (i > 0) {
          --i;
          ++it;
          if (it == end)
            it = begin;
        }
        onToolGroupChange(editor, *it);
      }
      break;
    }

    case WheelAction::Layer: {
      int deltaLayer = 0;
      if (preciseWheel == PreciseWheel::On) {
        if (dz < 0.0)
          deltaLayer = +1;
        else if (dz > 0.0)
          deltaLayer = -1;
      }
      else {
        deltaLayer = -dz;
      }

      const LayerList& layers = browsableLayers(editor);
      layer_t layer = initialLayer(editor) + deltaLayer;
      layer_t nlayers = layers.size();
      while (layer < 0)
        layer += nlayers;
      while (layer >= nlayers)
        layer -= nlayers;

      editor->setLayer(layers[layer]);
      break;
    }

    case WheelAction::InkType: {
      int ink = int(initialInkType(editor));
      int deltaInk = int(dz);
      if (preciseWheel == PreciseWheel::On) {
        if (dz < 0.0)
          deltaInk = +1;
        else if (dz > 0.0)
          deltaInk = -1;
      }
      ink += deltaInk;
      ink = std::clamp(ink,
                       int(tools::InkType::FIRST),
                       int(tools::InkType::LAST));

      App::instance()->contextBar()->setInkType(tools::InkType(ink));
      break;
    }

    case WheelAction::InkOpacity: {
      int opacity = initialInkOpacity(editor);
      adjust_value(preciseWheel, dz, opacity, 0, 255);

      tools::Tool* tool = getActiveTool();
      auto& toolPref = Preferences::instance().tool(tool);
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
          int opacity = initialLayerOpacity(editor);
          adjust_value(preciseWheel, dz, opacity, 0, 255);

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
          int opacity = initialCelOpacity(editor);
          adjust_value(preciseWheel, dz, opacity, 0, 255);

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

      Color c = initialFgColor();
      int a = c.getAlpha();
      adjust_value(preciseWheel, dz, a, 0, 255);
      c.setAlpha(a);

      changeFgColor(c);
      break;
    }

    case WheelAction::HslHue:
    case WheelAction::HslSaturation:
    case WheelAction::HslLightness: {
      disableQuickTool();

      Color c = initialFgColor();
      double
        h = c.getHslHue(),
        s = c.getHslSaturation(),
        l = c.getHslLightness();
      switch (wheelAction) {
        case WheelAction::HslHue:
          adjust_hue(preciseWheel, dz, h, 0.0, 360.0);
          break;
        case WheelAction::HslSaturation:
          adjust_unit(preciseWheel, dz, s);
          break;
        case WheelAction::HslLightness:
          adjust_unit(preciseWheel, dz, l);
          break;
      }

      changeFgColor(Color::fromHsl(std::clamp(h, 0.0, 360.0),
                                   std::clamp(s, 0.0, 1.0),
                                   std::clamp(l, 0.0, 1.0)));
      break;
    }

    case WheelAction::HsvHue:
    case WheelAction::HsvSaturation:
    case WheelAction::HsvValue: {
      disableQuickTool();

      Color c = initialFgColor();
      double
        h = c.getHsvHue(),
        s = c.getHsvSaturation(),
        v = c.getHsvValue();
      switch (wheelAction) {
        case WheelAction::HsvHue:
          adjust_hue(preciseWheel, dz, h, 0.0, 360.0);
          break;
        case WheelAction::HsvSaturation:
          adjust_unit(preciseWheel, dz, s);
          break;
        case WheelAction::HsvValue:
          adjust_unit(preciseWheel, dz, v);
          break;
      }

      changeFgColor(Color::fromHsv(std::clamp(h, 0.0, 360.0),
                                   std::clamp(s, 0.0, 1.0),
                                   std::clamp(v, 0.0, 1.0)));
      break;
    }

  }
}

bool StateWithWheelBehavior::onTouchMagnify(Editor* editor, ui::TouchMessage* msg)
{
  render::Zoom zoom = editor->zoom();
  zoom = render::Zoom::fromScale(
    zoom.internalScale() + zoom.internalScale() * msg->magnification());

  setZoom(editor, zoom, msg->position());
  return true;
}

bool StateWithWheelBehavior::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  tools::Ink* ink = editor->getCurrentEditorInk();
  auto theme = skin::SkinTheme::get(editor);

  if (ink) {
    // If the current tool change selection (e.g. rectangular marquee, etc.)
    if (ink->isSelection()) {
      editor->showBrushPreview(mouseScreenPos);
      return true;
    }
    else if (ink->isEyedropper()) {
      editor->showMouseCursor(
        kCustomCursor, theme->cursors.eyedropper());
      return true;
    }
    else if (ink->isZoom()) {
      editor->showMouseCursor(
        kCustomCursor, theme->cursors.magnifier());
      return true;
    }
    else if (ink->isScrollMovement()) {
      editor->showMouseCursor(kScrollCursor);
      return true;
    }
    else if (ink->isCelMovement()) {
      editor->showMouseCursor(kMoveCursor);
      return true;
    }
  }

  // Draw
  if (editor->canDraw()) {
    editor->showBrushPreview(mouseScreenPos);
  }
  // Forbidden
  else {
    editor->showMouseCursor(kForbiddenCursor);
  }

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

Color StateWithWheelBehavior::initialFgColor() const
{
  return ColorBar::instance()->getFgColor();
}

Color StateWithWheelBehavior::initialBgColor() const
{
  return ColorBar::instance()->getBgColor();
}

int StateWithWheelBehavior::initialFgTileIndex() const
{
  return ColorBar::instance()->getFgTile();
}

int StateWithWheelBehavior::initialBgTileIndex() const
{
  return ColorBar::instance()->getBgTile();
}

int StateWithWheelBehavior::initialBrushSize()
{
  tools::Tool* tool = getActiveTool();
  ToolPreferences::Brush& brush =
    Preferences::instance().tool(tool).brush;

  return brush.size();
}

int StateWithWheelBehavior::initialBrushAngle()
{
  tools::Tool* tool = getActiveTool();
  ToolPreferences::Brush& brush =
    Preferences::instance().tool(tool).brush;

  return brush.angle();
}

gfx::Point StateWithWheelBehavior::initialScroll(Editor* editor) const
{
  View* view = View::getView(editor);
  return view->viewScroll();
}

render::Zoom StateWithWheelBehavior::initialZoom(Editor* editor) const
{
  return editor->zoom();
}

doc::frame_t StateWithWheelBehavior::initialFrame(Editor* editor) const
{
  return editor->frame();
}

doc::layer_t StateWithWheelBehavior::initialLayer(Editor* editor) const
{
  return doc::find_layer_index(browsableLayers(editor), editor->layer());
}

tools::InkType StateWithWheelBehavior::initialInkType(Editor* editor) const
{
  tools::Tool* tool = getActiveTool();
  auto& toolPref = Preferences::instance().tool(tool);
  return toolPref.ink();
}

int StateWithWheelBehavior::initialInkOpacity(Editor* editor) const
{
  tools::Tool* tool = getActiveTool();
  auto& toolPref = Preferences::instance().tool(tool);
  return toolPref.opacity();
}

int StateWithWheelBehavior::initialCelOpacity(Editor* editor) const
{
  doc::Layer* layer = editor->layer();
  if (layer &&
      layer->isImage() &&
      layer->isEditable()) {
    if (Cel* cel = layer->cel(editor->frame()))
      return cel->opacity();
  }
  return 0;
}

int StateWithWheelBehavior::initialLayerOpacity(Editor* editor) const
{
  doc::Layer* layer = editor->layer();
  if (layer &&
      layer->isImage() &&
      layer->isEditable()) {
    return static_cast<doc::LayerImage*>(layer)->opacity();
  }
  else
    return 0;
}

tools::Tool* StateWithWheelBehavior::initialTool() const
{
  return getActiveTool();
}

void StateWithWheelBehavior::changeFgColor(Color c)
{
  ColorBar::instance()->setFgColor(c);
}

tools::Tool* StateWithWheelBehavior::getActiveTool() const
{
  disableQuickTool();
  return App::instance()->activeToolManager()->activeTool();
}

const doc::LayerList& StateWithWheelBehavior::browsableLayers(Editor* editor) const
{
  if (m_browsableLayers.empty())
    m_browsableLayers = editor->sprite()->allBrowsableLayers();
  return m_browsableLayers;
}

void StateWithWheelBehavior::disableQuickTool() const
{
  auto atm = App::instance()->activeToolManager();
  if (atm->quickTool()) {
    // As Ctrl key could active the Move tool, and Ctrl+mouse wheel can
    // change the size of the tool, we want to remove the quick tool so
    // the effect is for the selected tool.
    atm->newQuickToolSelectedFromEditor(nullptr);
  }
}

tools::Tool* StateWithWheelBehavior::getInitialToolInActiveGroup()
{
  if (!m_tool)
    m_tool = initialTool();
  return m_tool;
}

void StateWithWheelBehavior::onToolChange(tools::Tool* tool)
{
  ToolBar::instance()->selectTool(tool);
  m_tool = tool;
}

void StateWithWheelBehavior::onToolGroupChange(Editor* editor,
                                               tools::ToolGroup* group)
{
  ToolBar::instance()->selectToolGroup(group);

  // Update initial tool in active group as the group has just
  // changed. Useful when the same key modifiers are associated to
  // WheelAction::ToolSameGroup and WheelAction::ToolOtherGroup at the
  // same time.
  m_tool = getActiveTool();
}

} // namespace app
