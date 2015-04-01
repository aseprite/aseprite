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
#include "app/ui/tabs.h"
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

void Workspace::setTabsBar(Tabs* tabs)
{
  m_tabs = tabs;
  m_mainPanel.setTabsBar(tabs);
}

void Workspace::addView(WorkspaceView* view, int pos)
{
  m_mainPanel.addView(view, pos);
  m_activePanel = &m_mainPanel;
  m_views.push_back(view);

  setActiveView(view);
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
  for (Widget* child : getChildren())
    child->setBounds(rc);
}

void Workspace::setDropViewPreview(const gfx::Point& pos)
{
  WorkspacePanel* panel = getPanelAt(pos);

  if (m_dropPreviewPanel && m_dropPreviewPanel != panel)
    m_dropPreviewPanel->removeDropViewPreview();

  m_dropPreviewPanel = panel;

  if (m_dropPreviewPanel)
    m_dropPreviewPanel->setDropViewPreview(pos);
}

void Workspace::removeDropViewPreview()
{
  if (m_dropPreviewPanel)
    m_dropPreviewPanel->removeDropViewPreview();
}

bool Workspace::dropViewAt(const gfx::Point& pos, WorkspaceView* view)
{
  if (!m_dropPreviewPanel)
    return false;

  return m_dropPreviewPanel->dropViewAt(pos, getViewPanel(view), view);
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

} // namespace app
