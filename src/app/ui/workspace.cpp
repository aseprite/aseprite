// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/workspace.h"

#include "app/app.h"
#include "app/ui/input_chain.h"
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
  enableFlags(IGNORE_MOUSE);
  addChild(&m_mainPanel);

  InitTheme.connect(
    [this]{
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
      setBgColor(theme->colors.workspace());
    });
  initTheme();
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

bool Workspace::closeView(WorkspaceView* view, bool quitting)
{
  return view->onCloseView(this, quitting);
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

bool Workspace::canSelectOtherTab() const
{
  return m_activePanel->tabs()->canSelectOtherTab();
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

void Workspace::updateTabs()
{
  WidgetsList children = this->children();
  while (!children.empty()) {
    Widget* child = children.back();
    children.erase(--children.end());

    if (child->type() == WorkspacePanel::Type())
      static_cast<WorkspacePanel*>(child)->tabs()->updateTabs();

    for (auto subchild : child->children())
      children.push_back(subchild);
  }
}

void Workspace::onPaint(PaintEvent& ev)
{
  ev.graphics()->fillRect(bgColor(), clientBounds());
}

void Workspace::onResize(ui::ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  gfx::Rect rc = childrenBounds();
  for (auto child : children())
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

DropViewAtResult Workspace::dropViewAt(const gfx::Point& pos, WorkspaceView* view, bool clone)
{
  WorkspaceTabs* tabs = getTabsAt(pos);
  WorkspacePanel* panel = getPanelAt(pos);

  if (panel) {
    // Create new panel
    return panel->dropViewAt(pos, getViewPanel(view), view, clone);
  }
  else if (tabs && tabs != getViewPanel(view)->tabs()) {
    // Dock tab in other tabs
    WorkspacePanel* dropPanel = tabs->panel();
    ASSERT(dropPanel);

    int pos = tabs->getDropTabIndex();
    DropViewAtResult result;

    WorkspaceView* originalView = view;
    if (clone) {
      view = view->cloneWorkspaceView();
      result = DropViewAtResult::CLONED_VIEW;
    }
    else {
      removeView(view);
      result = DropViewAtResult::MOVED_TO_OTHER_PANEL;
    }

    addViewToPanel(dropPanel, view, true, pos);

    if (result == DropViewAtResult::CLONED_VIEW)
      view->onClonedFrom(originalView);

    return result;
  }
  else
    return DropViewAtResult::NOTHING;
}

void Workspace::addViewToPanel(WorkspacePanel* panel, WorkspaceView* view, bool from_drop, int pos)
{
  panel->addView(view, from_drop, pos);

  m_activePanel = panel;
  m_views.push_back(view);

  setActiveView(view);
  layout();
}

WorkspacePanel* Workspace::getViewPanel(WorkspaceView* view)
{
  Widget* widget = view->getContentWidget();
  while (widget) {
    if (widget->type() == WorkspacePanel::Type())
      return static_cast<WorkspacePanel*>(widget);

    widget = widget->parent();
  }
  return nullptr;
}

WorkspacePanel* Workspace::getPanelAt(const gfx::Point& pos)
{
  Widget* widget = manager()->pick(pos);
  while (widget) {
    if (widget->type() == WorkspacePanel::Type())
      return static_cast<WorkspacePanel*>(widget);

    widget = widget->parent();
  }
  return nullptr;
}

WorkspaceTabs* Workspace::getTabsAt(const gfx::Point& pos)
{
  Widget* widget = manager()->pick(pos);
  while (widget) {
    if (widget->type() == Tabs::Type())
      return static_cast<WorkspaceTabs*>(widget);

    widget = widget->parent();
  }
  return nullptr;
}

void Workspace::onNewInputPriority(InputChainElement* newElement,
                                   const ui::Message* msg)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    activeElement->onNewInputPriority(newElement, msg);
}

bool Workspace::onCanCut(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCanCut(ctx);
  else
    return false;
}

bool Workspace::onCanCopy(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCanCopy(ctx);
  else
    return false;
}

bool Workspace::onCanPaste(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCanPaste(ctx);
  else
    return false;
}

bool Workspace::onCanClear(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCanClear(ctx);
  else
    return false;
}

bool Workspace::onCut(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCut(ctx);
  else
    return false;
}

bool Workspace::onCopy(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onCopy(ctx);
  else
    return false;
}

bool Workspace::onPaste(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onPaste(ctx);
  else
    return false;
}

bool Workspace::onClear(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    return activeElement->onClear(ctx);
  else
    return false;
}

void Workspace::onCancel(Context* ctx)
{
  WorkspaceView* view = activeView();
  InputChainElement* activeElement = (view ? view->onGetInputChainElement(): nullptr);
  if (activeElement)
    activeElement->onCancel(ctx);
}

} // namespace app
