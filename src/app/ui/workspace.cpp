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

#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui/workspace_view.h"
#include "base/remove_from_container.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"

namespace app {

using namespace app::skin;
using namespace ui;

// static
WidgetType Workspace::Type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

Workspace::Workspace()
  : Widget(Workspace::Type())
  , m_mainPanel(WorkspacePanel::MAIN_PANEL)
  , m_tabs(nullptr)
  , m_activePanel(&m_mainPanel)
  , m_dropPreviewPanel(nullptr)
  , m_dropPreviewTabs(nullptr)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());

  addChild(&m_mainPanel);
}

Workspace::~Workspace()
{
  // No views at this point.
  ASSERT(m_views.empty());
}

void Workspace::setTabsBar(WorkspaceTabs* tabs)
{
  m_tabs = tabs;
  m_mainPanel.setTabsBar(tabs);
}

void Workspace::addView(WorkspaceView* view, int pos)
{
  addViewToPanel(&m_mainPanel, view, false, pos);
}

void Workspace::removeView(WorkspaceView* view)
{
  base::remove_from_container(m_views, view);

  WorkspacePanel* panel = getViewPanel(view);
  ASSERT(panel);
  if (panel)
    panel->removeView(view);
}

bool Workspace::closeView(WorkspaceView* view)
{
  return view->onCloseView(this);
}

WorkspaceView* Workspace::activeView()
{
  return (m_activePanel ? m_activePanel->activeView(): nullptr);
}

void Workspace::setActiveView(WorkspaceView* view)
{
  m_activePanel = getViewPanel(view);
  ASSERT(m_activePanel);
  if (!m_activePanel)
    return;

  m_activePanel->setActiveView(view);

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

void Workspace::setMainPanelAsActive()
{
  m_activePanel = &m_mainPanel;

  removeDropViewPreview();
  m_dropPreviewPanel = nullptr;
  m_dropPreviewTabs = nullptr;

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

void Workspace::selectNextTab()
{
  m_activePanel->tabs()->selectNextTab();
}

void Workspace::selectPreviousTab()
{
  m_activePanel->tabs()->selectPreviousTab();
}

void Workspace::duplicateActiveView()
{
  WorkspaceView* view = activeView();
  if (!view)
    return;

  WorkspaceView* clone = view->cloneWorkspaceView();
  if (!clone)
    return;

  WorkspacePanel* panel = getViewPanel(view);
  addViewToPanel(panel, clone, false, -1);
  clone->onClonedFrom(view);
  setActiveView(clone);
}

void Workspace::onPaint(PaintEvent& ev)
{
  ev.getGraphics()->fillRect(getBgColor(), getClientBounds());
}

void Workspace::onResize(ui::ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  gfx::Rect rc = getChildrenBounds();
  for (Widget* child : getChildren())
    child->setBounds(rc);
}

DropViewPreviewResult Workspace::setDropViewPreview(const gfx::Point& pos,
  WorkspaceView* view, WorkspaceTabs* tabs)
{
  TabView* tabView = dynamic_cast<TabView*>(view);
  WorkspaceTabs* newTabs = nullptr;
  WorkspacePanel* panel = getPanelAt(pos);
  if (!newTabs) {
    newTabs = getTabsAt(pos);
    // Drop preview is only to drop tabs from a different WorkspaceTabs.
    if (newTabs == tabs)
      newTabs = nullptr;
  }

  if (m_dropPreviewPanel && m_dropPreviewPanel != panel)
    m_dropPreviewPanel->removeDropViewPreview();
  if (m_dropPreviewTabs && m_dropPreviewTabs != newTabs)
    m_dropPreviewTabs->removeDropViewPreview();

  m_dropPreviewPanel = panel;
  m_dropPreviewTabs = newTabs;

  if (m_dropPreviewPanel)
    m_dropPreviewPanel->setDropViewPreview(pos, view);
  if (m_dropPreviewTabs)
    m_dropPreviewTabs->setDropViewPreview(pos, tabView);

  if (panel)
    return DropViewPreviewResult::DROP_IN_PANEL;
  else if (newTabs)
    return DropViewPreviewResult::DROP_IN_TABS;
  else
    return DropViewPreviewResult::FLOATING;
}

void Workspace::removeDropViewPreview()
{
  if (m_dropPreviewPanel) {
    m_dropPreviewPanel->removeDropViewPreview();
    m_dropPreviewPanel = nullptr;
  }

  if (m_dropPreviewTabs) {
    m_dropPreviewTabs->removeDropViewPreview();
    m_dropPreviewTabs = nullptr;
  }
}

bool Workspace::dropViewAt(const gfx::Point& pos, WorkspaceView* view)
{
  WorkspaceTabs* tabs = getTabsAt(pos);
  WorkspacePanel* panel = getPanelAt(pos);

  if (panel) {
    // Create new panel
    return panel->dropViewAt(pos, getViewPanel(view), view);
  }
  else if (tabs && tabs != getViewPanel(view)->tabs()) {
    // Dock tab in other tabs
    WorkspacePanel* dropPanel = tabs->panel();
    ASSERT(dropPanel);

    int pos = tabs->getDropTabIndex();

    removeView(view);
    addViewToPanel(dropPanel, view, true, pos);
    return true;
  }
  else
    return false;
}

void Workspace::addViewToPanel(WorkspacePanel* panel, WorkspaceView* view, bool from_drop, int pos)
{
  panel->addView(view, from_drop, pos);

  m_activePanel = panel;
  m_views.push_back(view);

  setActiveView(view);
}

WorkspacePanel* Workspace::getViewPanel(WorkspaceView* view)
{
  Widget* widget = view->getContentWidget();
  while (widget) {
    if (widget->getType() == WorkspacePanel::Type())
      return static_cast<WorkspacePanel*>(widget);

    widget = widget->getParent();
  }
  return nullptr;
}

WorkspacePanel* Workspace::getPanelAt(const gfx::Point& pos)
{
  Widget* widget = getManager()->pick(pos);
  while (widget) {
    if (widget->getType() == WorkspacePanel::Type())
      return static_cast<WorkspacePanel*>(widget);

    widget = widget->getParent();
  }
  return nullptr;
}

WorkspaceTabs* Workspace::getTabsAt(const gfx::Point& pos)
{
  Widget* widget = getManager()->pick(pos);
  while (widget) {
    if (widget->getType() == Tabs::Type())
      return static_cast<WorkspaceTabs*>(widget);

    widget = widget->getParent();
  }
  return nullptr;
}

} // namespace app
