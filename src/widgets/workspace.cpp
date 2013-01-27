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

#include "skin/skin_theme.h"
#include "widgets/workspace_view.h"

#include <algorithm>

using namespace ui;
using namespace widgets;

Workspace::Workspace()
  : Box(JI_VERTICAL)
  , m_activeView(NULL)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->getColor(ThemeColor::Workspace));
}

Workspace::~Workspace()
{
}

void Workspace::addView(WorkspaceView* view)
{
  m_views.push_back(view);

  addChild(view->getContentWidget());

  setActiveView(view);
}

void Workspace::removeView(WorkspaceView* view)
{
  WorkspaceViews::iterator it = std::find(m_views.begin(), m_views.end(), view);
  ASSERT(it != m_views.end());
  m_views.erase(it);

  removeChild(view->getContentWidget());

  setActiveView(NULL);
}

void Workspace::setActiveView(WorkspaceView* view)
{
  m_activeView = view;

  Widget* newContent = (view ? view->getContentWidget(): NULL);
  WidgetsList children = getChildren();
  UI_FOREACH_WIDGET(children, it) {
    if ((*it) != newContent)
      (*it)->setVisible(false);
  }

  if (newContent) {
    newContent->setExpansive(true);
    newContent->setVisible(true);
    newContent->requestFocus();
  }

  layout();
}

void Workspace::splitView(WorkspaceView* view, int orientation)
{
  // TODO
}

void Workspace::closeView(WorkspaceView* view)
{
  // TODO
}

void Workspace::makeUnique(WorkspaceView* view)
{
  // TODO
}
