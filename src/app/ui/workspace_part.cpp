// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/workspace_part.h"

#include "app/ui/workspace_view.h"
#include "app/ui/skin/skin_theme.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace ui;

WorkspacePart::WorkspacePart()
  : Box(JI_VERTICAL)
  , m_activeView(NULL)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->getColor(ThemeColor::Workspace));

  setExpansive(true);
}

WorkspacePart::~WorkspacePart()
{
}

void WorkspacePart::addView(WorkspaceView* view)
{
  m_views.push_back(view);

  addChild(view->getContentWidget());

  setActiveView(view);
}

void WorkspacePart::removeView(WorkspaceView* view)
{
  WorkspaceViews::iterator it = std::find(m_views.begin(), m_views.end(), view);
  ASSERT(it != m_views.end());
  it = m_views.erase(it);

  removeChild(view->getContentWidget());

  setActiveView((it != m_views.end() ?
                 *it : (!m_views.empty() ? m_views.front(): NULL)));
}

void WorkspacePart::setActiveView(WorkspaceView* view)
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

  if (m_activeView)
    m_activeView->onWorkspaceViewSelected();

  layout();
}

bool WorkspacePart::hasView(WorkspaceView* view)
{
  return (std::find(m_views.begin(), m_views.end(), view) != m_views.end());
}

} // namespace app
