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

#define ANI_DROPAREA_TICKS  4

using namespace app::skin;
using namespace ui;

Workspace::Workspace()
  : Widget(kGenericWidget)
  , m_tabsBar(nullptr)
  , m_activeView(nullptr)
  , m_dropArea(0)
  , m_leftTime(0)
  , m_rightTime(0)
  , m_topTime(0)
  , m_bottomTime(0)
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
  if (animation() == ANI_DROPAREA) {
    double left = double(m_leftTime) / double(ANI_DROPAREA_TICKS);
    double top = double(m_topTime) / double(ANI_DROPAREA_TICKS);
    double right = double(m_rightTime) / double(ANI_DROPAREA_TICKS);
    double bottom = double(m_bottomTime) / double(ANI_DROPAREA_TICKS);
    double threshold = getDropThreshold();

    rc.x += int(inbetween(0.0, threshold, left));
    rc.y += int(inbetween(0.0, threshold, top));
    rc.w -= int(inbetween(0.0, threshold, left) + inbetween(0.0, threshold, right));
    rc.h -= int(inbetween(0.0, threshold, top) + inbetween(0.0, threshold, bottom));
  }

  for (Widget* child : getChildren())
    child->setBounds(rc);
}

void Workspace::setDropViewPreview(const gfx::Point& pos)
{
  int newDropArea = calculateDropArea(pos);
  if (newDropArea != m_dropArea) {
    m_dropArea = newDropArea;
    startAnimation(ANI_DROPAREA, ANI_DROPAREA_TICKS);
  }
}

void Workspace::removeDropViewPreview(const gfx::Point& pos)
{
  if (m_dropArea) {
    m_dropArea = 0;
    startAnimation(ANI_DROPAREA, ANI_DROPAREA_TICKS);
  }
}

void Workspace::onAnimationStop()
{
  if (animation() == ANI_DROPAREA)
    layout();
}

void Workspace::onAnimationFrame()
{
  if (animation() == ANI_DROPAREA) {
    adjustTime(m_leftTime, JI_LEFT);
    adjustTime(m_topTime, JI_TOP);
    adjustTime(m_rightTime, JI_RIGHT);
    adjustTime(m_bottomTime, JI_BOTTOM);
    layout();
  }
}

void Workspace::adjustTime(int& time, int flag)
{
  if (m_dropArea & flag) {
    if (time < ANI_DROPAREA_TICKS)
      ++time;
  }
  else if (time > 0)
    --time;
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
