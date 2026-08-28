// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/floating_toolbox.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_group.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"

namespace app {

using namespace ui;
using namespace app::skin;

FloatingToolbox::FloatingToolbox()
  : FloatingWindow(Strings::floating_toolbox_title(), "FloatingToolbox")
  , m_hotTool(nullptr)
{
  fillToolList();
  App::instance()->activeToolManager()->add_observer(this);
}

FloatingToolbox::~FloatingToolbox()
{
  App::instance()->activeToolManager()->remove_observer(this);
}

void FloatingToolbox::fillToolList()
{
  m_tools.clear();
  auto* toolbox = App::instance()->toolBox();
  for (auto* tool : *toolbox)
    m_tools.push_back(tool);
}

gfx::Size FloatingToolbox::getToolIconSize() const
{
  auto* theme = SkinTheme::get(this);
  os::Surface* icon = theme->getToolIcon("configuration");
  if (icon)
    return gfx::Size(icon->width(), icon->height());
  return gfx::Size(16, 16) * guiscale();
}

int FloatingToolbox::getColumns(int width) const
{
  int cw = getToolIconSize().w;
  if (cw <= 0)
    return 1;
  return std::max(1, width / cw);
}

gfx::Rect FloatingToolbox::toolBounds(int index) const
{
  gfx::Size iconSize = getToolIconSize();
  gfx::Rect client = clientChildrenBounds();
  int cols = getColumns(client.w);
  int col = index % cols;
  int row = index / cols;
  return gfx::Rect(client.x + col * iconSize.w, client.y + row * iconSize.h, iconSize.w, iconSize.h);
}

tools::Tool* FloatingToolbox::hitTestTool(const gfx::Point& pt) const
{
  for (int i = 0; i < (int)m_tools.size(); ++i) {
    if (toolBounds(i).contains(pt))
      return m_tools[i];
  }
  return nullptr;
}

void FloatingToolbox::onPaint(ui::PaintEvent& ev)
{
  auto* g = ev.graphics();
  auto* theme = SkinTheme::get(this);
  auto* activeTool = App::instance()->activeTool();

  Window::onPaint(ev);

  for (int i = 0; i < (int)m_tools.size(); ++i) {
    auto* tool = m_tools[i];
    gfx::Rect rc = toolBounds(i);
    rc.offset(-origin());

    SkinPartPtr nw;
    if (tool == activeTool || tool == m_hotTool)
      nw = theme->parts.toolbuttonHot();
    else
      nw = theme->parts.toolbuttonNormal();

    theme->drawRect(g, rc, nw.get());

    os::Surface* icon = theme->getToolIcon(tool->getId().c_str());
    if (icon) {
      g->drawRgbaSurface(icon,
                         guiscaled_center(rc.x, rc.w, icon->width()),
                         guiscaled_center(rc.y, rc.h, icon->height()));
    }
  }
}

bool FloatingToolbox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage: {
      auto* mouseMsg = static_cast<const MouseMessage*>(msg);
      gfx::Point pt = mouseMsg->positionForDisplay(display());
      auto* tool = hitTestTool(pt);
      if (tool)
        App::instance()->activeToolManager()->setSelectedTool(tool);
      break;
    }

    case kMouseMoveMessage: {
      auto* mouseMsg = static_cast<const MouseMessage*>(msg);
      gfx::Point pt = mouseMsg->positionForDisplay(display());
      auto* tool = hitTestTool(pt);
      if (tool != m_hotTool) {
        m_hotTool = tool;
        invalidate();
      }
      break;
    }

    case kMouseLeaveMessage:
      if (m_hotTool) {
        m_hotTool = nullptr;
        invalidate();
      }
      break;
  }

  return FloatingWindow::onProcessMessage(msg);
}

void FloatingToolbox::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size iconSize = getToolIconSize();
  int cols = 4;
  int rows = ((int)m_tools.size() + cols - 1) / cols;
  gfx::Border b = border();
  ev.setSizeHint(gfx::Size(cols * iconSize.w + b.width(), rows * iconSize.h + b.height()));
}

void FloatingToolbox::onResize(ui::ResizeEvent& ev)
{
  Window::onResize(ev);
  invalidate();
}

void FloatingToolbox::onActiveToolChange(tools::Tool* tool)
{
  invalidate();
}

void FloatingToolbox::onSelectedToolChange(tools::Tool* tool)
{
  invalidate();
}

} // namespace app
