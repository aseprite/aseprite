/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "widgets/workspace.h"

#include "app.h"
#include "skin/skin_theme.h"
#include "ui/splitter.h"
#include "widgets/main_window.h"
#include "widgets/tabs.h"
#include "widgets/workspace_part.h"
#include "widgets/workspace_view.h"

#include <algorithm>
#include <queue>

using namespace ui;
using namespace widgets;

Workspace::Workspace()
  : Box(JI_VERTICAL)
  , m_activePart(new WorkspacePart)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->getColor(ThemeColor::Workspace));

  addChild(m_activePart);
}

Workspace::~Workspace()
{
  // No views at this point.
  ASSERT(m_views.empty());
}

void Workspace::addView(WorkspaceView* view)
{
  m_views.push_back(view);

  m_activePart->addView(view);

  App::instance()->getMainWindow()->getTabsBar()->addTab(dynamic_cast<TabView*>(view));

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

void Workspace::removeView(WorkspaceView* view)
{
  WorkspaceViews::iterator it = std::find(m_views.begin(), m_views.end(), view);
  ASSERT(it != m_views.end());
  m_views.erase(it);

  m_activePart->removeView(view);
  if (m_activePart->getViewCount() == 0 &&
      m_activePart->getParent() != this) {
    m_activePart = destroyPart(m_activePart);
  }

  App::instance()->getMainWindow()->getTabsBar()->removeTab(dynamic_cast<TabView*>(view));

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

WorkspaceView* Workspace::getActiveView()
{
  ASSERT(m_activePart != NULL);
  return m_activePart->getActiveView();
}

void Workspace::setActiveView(WorkspaceView* view)
{
  ASSERT(view != NULL);

  WorkspacePart* viewPart =
    static_cast<WorkspacePart*>(view->getContentWidget()->getParent());

  viewPart->setActiveView(view);

  m_activePart = viewPart;
  ActiveViewChanged();          // Fire ActiveViewChanged event
}

void Workspace::splitView(WorkspaceView* view, int orientation)
{
  WorkspacePart* viewPart =
    static_cast<WorkspacePart*>(view->getContentWidget()->getParent());

  // Create a new splitter to add new WorkspacePart on it: the given
  // "viewPart" and a new part named "newPart".
  Splitter* splitter = new Splitter(Splitter::ByPercentage, orientation);
  splitter->setExpansive(true);

  // Create the new part to contain the cloned view (see below, "newView").
  WorkspacePart* newPart = new WorkspacePart();

  // Replace the "viewPart" with the "splitter".
  Widget* parent = viewPart->getParent();
  parent->replaceChild(viewPart, splitter);
  splitter->addChild(viewPart);
  splitter->addChild(newPart);

  // The new part is the active one.
  m_activePart = newPart;

  // Clone the workspace view, and add it to the active part (newPart)
  // using Workspace::addView().
  WorkspaceView* newView = view->cloneWorkspaceView();
  addView(newView);
  setActiveView(newView);

  layout();

  newView->onClonedFrom(view);

  ActiveViewChanged();          // Fire ActiveViewChanged event
}

WorkspacePart* Workspace::destroyPart(WorkspacePart* part)
{
  ASSERT(part != NULL);
  ASSERT(part->getViewCount() == 0);

  Widget* splitter = part->getParent();
  ASSERT(splitter != this);
  ASSERT(splitter->getChildren().size() == 2);
  splitter->removeChild(part);
  delete part;
  ASSERT(splitter->getChildren().size() == 1);

  Widget* otherWidget = splitter->getFirstChild();
  WorkspacePart* otherPart = dynamic_cast<WorkspacePart*>(otherWidget);
  if (otherPart == NULL) {
    Widget* widget = otherWidget;
    for (;;) {
      otherPart = widget->findFirstChildByType<WorkspacePart>();
      if (otherPart != NULL)
        break;

      widget = widget->getFirstChild();
    }
  }
  ASSERT(otherPart != NULL);

  splitter->removeChild(otherWidget);
  splitter->getParent()->replaceChild(splitter, otherWidget);
  delete splitter;

  layout();

  return otherPart;
}

void Workspace::makeUnique(WorkspaceView* view)
{
  std::vector<WorkspacePart*> parts;
  std::queue<Widget*> remaining;
  remaining.push(getFirstChild());
  while (!remaining.empty()) {
    Widget* widget = remaining.front();
    remaining.pop();

    WorkspacePart* part = dynamic_cast<WorkspacePart*>(widget);
    if (part && part->getParent() != this) {
      while (part->getActiveView()) {
        part->removeView(part->getActiveView());
      }
      parts.push_back(part);
    }
    else {
      UI_FOREACH_WIDGET(widget->getChildren(), it) {
        remaining.push(*it);
      }
    }
  }

  for (std::vector<WorkspacePart*>::iterator
         it = parts.begin(), end = parts.end(); it != end; ++it) {
    WorkspacePart* part = *it;
    if (part->getParent() != this)
      destroyPart(part);
  }

  WorkspacePart* uniquePart = dynamic_cast<WorkspacePart*>(getFirstChild());
  ASSERT(uniquePart != NULL);
  m_activePart = uniquePart;

  for (WorkspaceViews::iterator it = m_views.begin(), end = m_views.end(); it != end; ++it) {
    WorkspaceView* v = *it;
    if (!v->getContentWidget()->getParent())
      uniquePart->addView(v);
  }

  setActiveView(view);
}
