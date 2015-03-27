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
  , m_dropPreview(false)
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

  gfx::Rect cpos = getChildrenBounds();

  // Preview to drop tabs in workspace
  if (m_dropPreview && cpos.contains(m_dropPos)) {
    int left = ABS(cpos.x - m_dropPos.x);
    int top = ABS(cpos.y - m_dropPos.y);
    int right = ABS(cpos.x + cpos.w - m_dropPos.x);
    int bottom = ABS(cpos.y + cpos.h - m_dropPos.y);
    int threshold = 32*guiscale();
    if (threshold > cpos.w/2) threshold = cpos.w/2;
    if (threshold > cpos.h/2) threshold = cpos.h/2;

    if (left < threshold && left < right && left < top && left < bottom) {
      cpos.x += threshold;
      cpos.w -= threshold;
    }
    else if (top < threshold && top < left && top < right && top < bottom) {
      cpos.y += threshold;
      cpos.h -= threshold;
    }
    else if (right < threshold && right < left && right < top && right < bottom) {
      cpos.w -= threshold;
    }
    else if (bottom < threshold && bottom < left && bottom < top && bottom < right) {
      cpos.h -= threshold;
    }
  }

  for (Widget* child : getChildren())
    child->setBounds(cpos);
}

void Workspace::setDropViewPreview(const gfx::Point& pos)
{
  m_dropPos = pos;
  m_dropPreview = true;

  layout();
}

void Workspace::removeDropViewPreview(const gfx::Point& pos)
{
  m_dropPreview = false;
  layout();
}

void Workspace::dropViewAt(const gfx::Point& pos, WorkspaceView* view)
{
}

} // namespace app
