// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/active_tool.h"

#include "app/color.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool_observer.h"
#include "app/tools/ink.h"
#include "app/tools/pointer.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"

namespace app {
namespace tools {

class ActiveToolChangeTrigger {
public:
  ActiveToolChangeTrigger(ActiveToolManager* manager)
    : m_manager(manager)
    , m_oldTool(manager->activeTool()) {
  }

  ~ActiveToolChangeTrigger() {
    Tool* newTool = m_manager->activeTool();
    if (m_oldTool != newTool) {
      m_manager->notify_observers(
        &ActiveToolObserver::onActiveToolChange, newTool);
    }
  }

private:
  ActiveToolManager* m_manager;
  Tool* m_oldTool;
};

ActiveToolManager::ActiveToolManager(ToolBox* toolbox)
  : m_toolbox(toolbox)
  , m_quickTool(nullptr)
  , m_rightClick(false)
  , m_rightClickTool(nullptr)
  , m_rightClickInk(nullptr)
  , m_proximityTool(nullptr)
  , m_selectedTool(m_toolbox->getToolById(WellKnownTools::Pencil)) // "pencil" is the active tool by default
{
}

Tool* ActiveToolManager::activeTool() const
{
  if (m_quickTool)
    return m_quickTool;

  if (m_rightClickTool)
    return m_rightClickTool;

  if (m_proximityTool)
    return m_proximityTool;

  // Active tool should never returns null
  ASSERT(m_selectedTool);
  return m_selectedTool;
}

Ink* ActiveToolManager::activeInk() const
{
  if (!m_quickTool && m_rightClickInk)
    return m_rightClickInk;

  Tool* tool = activeTool();
  Ink* ink = tool->getInk(m_rightClick ? 1: 0);
  if (ink->isPaint() && !ink->isEffect()) {
    const tools::InkType inkType = Preferences::instance().tool(tool).ink();
    app::Color color;
#ifdef ENABLE_UI
    ColorBar* colorbar = ColorBar::instance();
    color = (m_rightClick ? colorbar->getBgColor():
                            colorbar->getFgColor());
#endif
    ink = adjustToolInkDependingOnSelectedInkType(ink, inkType, color);
  }

  return ink;
}

Ink* ActiveToolManager::adjustToolInkDependingOnSelectedInkType(
  Ink* ink,
  const InkType inkType,
  const app::Color& color) const
{
  if (ink->isPaint() && !ink->isEffect()) {
    const char* id = nullptr;
    switch (inkType) {
      case tools::InkType::SIMPLE:
        id = tools::WellKnownInks::Paint;
        if (color.getAlpha() == 0)
          id = tools::WellKnownInks::PaintCopy;
        break;
      case tools::InkType::ALPHA_COMPOSITING:
        id = tools::WellKnownInks::Paint;
        break;
      case tools::InkType::COPY_COLOR:
        id = tools::WellKnownInks::PaintCopy;
        break;
      case tools::InkType::LOCK_ALPHA:
        id = tools::WellKnownInks::PaintLockAlpha;
        break;
      case tools::InkType::SHADING:
        id = tools::WellKnownInks::Shading;
        break;
    }
    if (id)
      ink = m_toolbox->getInkById(id);
  }
  return ink;
}

Tool* ActiveToolManager::quickTool() const
{
  return m_quickTool;
}

Tool* ActiveToolManager::selectedTool() const
{
  return m_selectedTool;
}

void ActiveToolManager::newToolSelectedInToolBar(Tool* tool)
{
  ActiveToolChangeTrigger trigger(this);
  m_selectedTool = tool;
}

void ActiveToolManager::newQuickToolSelectedFromEditor(Tool* tool)
{
  ActiveToolChangeTrigger trigger(this);
  m_quickTool = tool;
}

void ActiveToolManager::regularTipProximity()
{
  if (m_proximityTool != nullptr) {
    ActiveToolChangeTrigger trigger(this);
    m_proximityTool = nullptr;
  }
}

void ActiveToolManager::eraserTipProximity()
{
  Tool* eraser = m_toolbox->getToolById(WellKnownTools::Eraser);
  if (m_proximityTool != eraser) {
    ActiveToolChangeTrigger trigger(this);
    m_proximityTool = eraser;
  }
}

void ActiveToolManager::pressButton(const Pointer& pointer)
{
  ActiveToolChangeTrigger trigger(this);
  Tool* tool = nullptr;
  Ink* ink = nullptr;

  if (pointer.button() == Pointer::Right) {
    m_rightClick = true;

    if (isToolAffectedByRightClickMode(activeTool())) {
      switch (Preferences::instance().editor.rightClickMode()) {
        case app::gen::RightClickMode::PAINT_BGCOLOR:
          // Do nothing, use the active tool
          break;
        case app::gen::RightClickMode::PICK_FGCOLOR:
          tool = m_toolbox->getToolById(WellKnownTools::Eyedropper);
          ink = m_toolbox->getInkById(tools::WellKnownInks::PickFg);
          break;
        case app::gen::RightClickMode::ERASE:
          tool = m_toolbox->getToolById(WellKnownTools::Eraser);
          ink = m_toolbox->getInkById(tools::WellKnownInks::Eraser);
          break;
        case app::gen::RightClickMode::SCROLL:
          tool = m_toolbox->getToolById(WellKnownTools::Hand);
          ink = m_toolbox->getInkById(tools::WellKnownInks::Scroll);
          break;
        case app::gen::RightClickMode::RECTANGULAR_MARQUEE:
          tool = m_toolbox->getToolById(WellKnownTools::RectangularMarquee);
          ink = m_toolbox->getInkById(tools::WellKnownInks::Selection);
          break;
        case app::gen::RightClickMode::LASSO:
          tool = m_toolbox->getToolById(WellKnownTools::Lasso);
          ink = m_toolbox->getInkById(tools::WellKnownInks::Selection);
          break;
        case app::gen::RightClickMode::SELECT_LAYER_AND_MOVE:
          tool = m_toolbox->getToolById(WellKnownTools::Move);
          ink = m_toolbox->getInkById(tools::WellKnownInks::SelectLayerAndMove);
          break;
      }
    }
  }
  else {
    m_rightClick = false;
  }

  m_rightClickTool = tool;
  m_rightClickInk = ink;
}

void ActiveToolManager::releaseButtons()
{
  ActiveToolChangeTrigger trigger(this);

  m_rightClick = false;
  m_rightClickTool = nullptr;
  m_rightClickInk = nullptr;
}

void ActiveToolManager::setSelectedTool(Tool* tool)
{
  ActiveToolChangeTrigger trigger(this);

  m_selectedTool = tool;
  notify_observers(&ActiveToolObserver::onSelectedToolChange, tool);
}

// static
bool ActiveToolManager::isToolAffectedByRightClickMode(Tool* tool)
{
  bool shadingMode = (Preferences::instance().tool(tool).ink() == InkType::SHADING);
  return
    ((tool->getInk(0)->isPaint() && !shadingMode) ||
     (tool->getInk(0)->isEffect())) &&
    (!tool->getInk(0)->isEraser()) &&
    (!tool->getInk(0)->isSelection());
}

} // namespace tools
} // namespace app
