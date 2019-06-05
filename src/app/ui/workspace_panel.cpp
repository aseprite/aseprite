// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/workspace_panel.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui/workspace_view.h"
#include "base/remove_from_container.h"
#include "ui/box.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/splitter.h"

namespace app {

#define ANI_DROPAREA_TICKS  4

using namespace app::skin;
using namespace ui;

// static
WidgetType WorkspacePanel::Type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

WorkspacePanel::WorkspacePanel(PanelType panelType)
  : Widget(WorkspacePanel::Type())
  , m_panelType(panelType)
  , m_tabs(nullptr)
  , m_activeView(nullptr)
  , m_dropArea(0)
  , m_leftTime(0)
  , m_rightTime(0)
  , m_topTime(0)
  , m_bottomTime(0)
{
  enableFlags(IGNORE_MOUSE);
  InitTheme.connect(
    [this]{
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
      setBgColor(theme->colors.workspace());
    });
  initTheme();
}

WorkspacePanel::~WorkspacePanel()
{
  // No views at this point.
  ASSERT(m_views.empty());
}

void WorkspacePanel::setTabsBar(WorkspaceTabs* tabs)
{
  m_tabs = tabs;
  m_tabs->setPanel(this);
}

void WorkspacePanel::addView(WorkspaceView* view, bool from_drop, int pos)
{
  if (pos < 0)
    m_views.push_back(view);
  else
    m_views.insert(m_views.begin()+pos, view);

  if (m_tabs)
    m_tabs->addTab(dynamic_cast<TabView*>(view), from_drop, pos);

  // Insert the view content as a hidden widget.
  Widget* content = view->getContentWidget();
  content->setVisible(false);
  addChild(content);

  setActiveView(view);
}

void WorkspacePanel::removeView(WorkspaceView* view)
{
  base::remove_from_container(m_views, view);

  Widget* content = view->getContentWidget();
  ASSERT(hasChild(content));
  removeChild(content);

  // Remove related tab.
  if (m_tabs) {
    m_tabs->removeTab(dynamic_cast<TabView*>(view), true);

    // The selected
    TabView* tabView = m_tabs->getSelectedTab();
    view = dynamic_cast<WorkspaceView*>(tabView);
  }
  else
    view = nullptr;

  setActiveView(view);
  if (!view)
    getWorkspace()->setMainPanelAsActive();

  // Destroy this panel
  if (m_views.empty() && m_panelType == SUB_PANEL) {
    Widget* self = parent();
    ASSERT(self->type() == kBoxWidget);

    Widget* splitter = self->parent();
    ASSERT(splitter->type() == kSplitterWidget);

    Widget* parent = splitter->parent();

    Widget* side =
      (splitter->firstChild() == self ?
        splitter->lastChild():
        splitter->firstChild());

    splitter->removeChild(side);
    parent->replaceChild(splitter, side);

    self->deferDelete();

    parent->layout();
  }
}

WorkspaceView* WorkspacePanel::activeView()
{
  return m_activeView;
}

void WorkspacePanel::setActiveView(WorkspaceView* view)
{
  m_activeView = view;

  for (auto v : m_views)
    v->getContentWidget()->setVisible(v == view);

  if (m_tabs && view)
    m_tabs->selectTab(dynamic_cast<TabView*>(view));

  adjustActiveViewBounds();

  if (m_activeView)
    m_activeView->onWorkspaceViewSelected();
}

void WorkspacePanel::onPaint(PaintEvent& ev)
{
  ev.graphics()->fillRect(bgColor(), clientBounds());
}

void WorkspacePanel::onResize(ui::ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());
  adjustActiveViewBounds();
}

void WorkspacePanel::adjustActiveViewBounds()
{
  gfx::Rect rc = childrenBounds();

  // Preview to drop tabs in workspace
  if (m_leftTime+m_topTime+m_rightTime+m_bottomTime > 1e-4) {
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

  for (auto child : children())
    if (child->isVisible())
      child->setBounds(rc);
}

void WorkspacePanel::setDropViewPreview(const gfx::Point& pos, WorkspaceView* view)
{
  int newDropArea = calculateDropArea(pos);
  if (newDropArea != m_dropArea) {
    m_dropArea = newDropArea;
    startAnimation(ANI_DROPAREA, ANI_DROPAREA_TICKS);
  }
}

void WorkspacePanel::removeDropViewPreview()
{
  if (m_dropArea) {
    m_dropArea = 0;
    startAnimation(ANI_DROPAREA, ANI_DROPAREA_TICKS);
  }
}

void WorkspacePanel::onAnimationStop(int animation)
{
  if (animation == ANI_DROPAREA)
    layout();
}

void WorkspacePanel::onAnimationFrame()
{
  if (animation() == ANI_DROPAREA) {
    adjustTime(m_leftTime, LEFT);
    adjustTime(m_topTime, TOP);
    adjustTime(m_rightTime, RIGHT);
    adjustTime(m_bottomTime, BOTTOM);
    layout();
  }
}

void WorkspacePanel::adjustTime(int& time, int flag)
{
  if (m_dropArea & flag) {
    if (time < ANI_DROPAREA_TICKS)
      ++time;
  }
  else if (time > 0)
    --time;
}

DropViewAtResult WorkspacePanel::dropViewAt(const gfx::Point& pos, WorkspacePanel* from, WorkspaceView* view, bool clone)
{
  int dropArea = calculateDropArea(pos);
  if (!dropArea)
    return DropViewAtResult::NOTHING;

  // If we're dropping the view in the same panel, and it's the only
  // view in the panel: We cannot drop the view in the panel (because
  // if we remove the last view, the panel will be destroyed).
  if (!clone && from == this && m_views.size() == 1)
    return DropViewAtResult::NOTHING;

  int splitterAlign = 0;
  if (dropArea & (LEFT | RIGHT)) splitterAlign = HORIZONTAL;
  else if (dropArea & (TOP | BOTTOM)) splitterAlign = VERTICAL;

  ASSERT(from);
  DropViewAtResult result;
  Workspace* workspace = getWorkspace();
  WorkspaceView* originalView = view;
  if (clone) {
    view = view->cloneWorkspaceView();
    result = DropViewAtResult::CLONED_VIEW;
  }
  else {
    workspace->removeView(view);
    result = DropViewAtResult::MOVED_TO_OTHER_PANEL;
  }

  WorkspaceTabs* newTabs = new WorkspaceTabs(m_tabs->getDelegate());
  WorkspacePanel* newPanel = new WorkspacePanel(SUB_PANEL);
  newTabs->setDockedStyle();
  newPanel->setTabsBar(newTabs);
  newPanel->setExpansive(true);

  Widget* self = this;
  VBox* side = new VBox;
  side->InitTheme.connect(
    [side]{
      side->noBorderNoChildSpacing();
    });
  side->initTheme();
  side->addChild(newTabs);
  side->addChild(newPanel);

  Splitter* splitter = new Splitter(Splitter::ByPercentage, splitterAlign);
  splitter->setExpansive(true);
  splitter->InitTheme.connect(
    [splitter]{
      splitter->setStyle(SkinTheme::instance()->styles.workspaceSplitter());
    });
  splitter->initTheme();

  Widget* parent = this->parent();
  if (parent->type() == kBoxWidget) {
    self = parent;
    parent = self->parent();
    ASSERT(parent->type() == kSplitterWidget);
  }
  if (parent->type() == Workspace::Type() ||
      parent->type() == kSplitterWidget) {
    parent->replaceChild(self, splitter);
  }
  else {
    ASSERT(false);
  }

  double sideSpace;
  if (m_panelType == MAIN_PANEL)
    sideSpace = 30;
  else
    sideSpace = 50;

  switch (dropArea) {
    case LEFT:
    case TOP:
      splitter->setPosition(sideSpace);
      splitter->addChild(side);
      splitter->addChild(self);
      break;
    case RIGHT:
    case BOTTOM:
      splitter->setPosition(100-sideSpace);
      splitter->addChild(self);
      splitter->addChild(side);
      break;
  }

  workspace->addViewToPanel(newPanel, view, true, -1);
  parent->layout();

  if (result == DropViewAtResult::CLONED_VIEW)
    view->onClonedFrom(originalView);

  return result;
}

int WorkspacePanel::calculateDropArea(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  if (rc.contains(pos)) {
    int left = ABS(rc.x - pos.x);
    int top = ABS(rc.y - pos.y);
    int right = ABS(rc.x + rc.w - pos.x);
    int bottom = ABS(rc.y + rc.h - pos.y);
    int threshold = getDropThreshold();

    if (left < threshold && left < right && left < top && left < bottom) {
      return LEFT;
    }
    else if (top < threshold && top < left && top < right && top < bottom) {
      return TOP;
    }
    else if (right < threshold && right < left && right < top && right < bottom) {
      return RIGHT;
    }
    else if (bottom < threshold && bottom < left && bottom < top && bottom < right) {
      return BOTTOM;
    }
  }

  return 0;
}

int WorkspacePanel::getDropThreshold() const
{
  gfx::Rect cpos = childrenBounds();
  int threshold = 32*guiscale();
  if (threshold > cpos.w/2) threshold = cpos.w/2;
  if (threshold > cpos.h/2) threshold = cpos.h/2;
  return threshold;
}

Workspace* WorkspacePanel::getWorkspace()
{
  Widget* widget = this;
  while (widget) {
    if (widget->type() == Workspace::Type())
      return static_cast<Workspace*>(widget);

    widget = widget->parent();
  }
  return nullptr;
}

} // namespace app
