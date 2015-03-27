// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/workspace.h"

#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "base/remove_from_container.h"

#include <algorithm>
#include <queue>

namespace app {

using namespace app::skin;
using namespace ui;

Workspace::Workspace()
  : Widget(kGenericWidget)
  , m_tabsBar(nullptr)
  , m_activeView(nullptr)
  , m_dropArea(0)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());
}

Workspace::~Workspace()
{
  // No views at this point.
  ASSERT(m_views.empty());
}

void Workspace::setTabsBar(Tabs* tabsBar)
{
  m_tabsBar = tabsBar;
}

void Workspace::addView(WorkspaceView* view, int pos)
{
  if (pos < 0)
    m_views.push_back(view);
  else
    m_views.insert(m_views.begin()+pos, view);

  m_tabsBar->addTab(dynamic_cast<TabView*>(view), pos);
  setActiveView(view);
}

void Workspace::removeView(WorkspaceView* view)
{
  base::remove_from_container(m_views, view);

  Widget* content = view->getContentWidget();
  if (content->getParent())
    content->getParent()->removeChild(content);

  // Remove related tab.
  m_tabsBar->removeTab(dynamic_cast<TabView*>(view));

  TabView* tabView = m_tabsBar->getSelectedTab();
  setActiveView(dynamic_cast<WorkspaceView*>(tabView));
}

bool Workspace::closeView(WorkspaceView* view)
{
  return view->onCloseView(this);
}

WorkspaceView* Workspace::activeView()
{
  return m_activeView;
}

void Workspace::setActiveView(WorkspaceView* view)
{
  m_activeView = view;

  removeAllChildren();
  if (view)
    addChild(view->getContentWidget());

  layout();

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

void Workspace::onPaint(PaintEvent& ev)
{
  ev.getGraphics()->fillRect(getBgColor(), getClientBounds());
}

void Workspace::onResize(ui::ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  gfx::Rect rc = getChildrenBounds();

  // Preview to drop tabs in workspace
  int threshold = getDropThreshold();
  switch (m_dropArea) {
    case JI_LEFT:
      rc.x += threshold;
      rc.w -= threshold;
      break;
    case JI_TOP:
      rc.y += threshold;
      rc.h -= threshold;
      break;
    case JI_RIGHT:
      rc.w -= threshold;
      break;
    case JI_BOTTOM:
      rc.h -= threshold;
      break;
  }

  for (Widget* child : getChildren())
    child->setBounds(rc);
}

void Workspace::setDropViewPreview(const gfx::Point& pos)
{
  int newDropArea = calculateDropArea(pos);
  if (newDropArea != m_dropArea) {
    m_dropArea = newDropArea;
    layout();
  }
}

void Workspace::removeDropViewPreview(const gfx::Point& pos)
{
  if (m_dropArea) {
    m_dropArea = 0;
    layout();
  }
}

void Workspace::dropViewAt(const gfx::Point& pos, WorkspaceView* view)
{
}

int Workspace::calculateDropArea(const gfx::Point& pos) const
{
  gfx::Rect rc = getChildrenBounds();
  if (rc.contains(pos)) {
    int left = ABS(rc.x - pos.x);
    int top = ABS(rc.y - pos.y);
    int right = ABS(rc.x + rc.w - pos.x);
    int bottom = ABS(rc.y + rc.h - pos.y);
    int threshold = getDropThreshold();

    if (left < threshold && left < right && left < top && left < bottom) {
      return JI_LEFT;
    }
    else if (top < threshold && top < left && top < right && top < bottom) {
      return JI_TOP;
    }
    else if (right < threshold && right < left && right < top && right < bottom) {
      return JI_RIGHT;
    }
    else if (bottom < threshold && bottom < left && bottom < top && bottom < right) {
      return JI_BOTTOM;
    }
  }

  return 0;
}

int Workspace::getDropThreshold() const
{
  gfx::Rect cpos = getChildrenBounds();
  int threshold = 32*guiscale();
  if (threshold > cpos.w/2) threshold = cpos.w/2;
  if (threshold > cpos.h/2) threshold = cpos.h/2;
  return threshold;
}

} // namespace app
