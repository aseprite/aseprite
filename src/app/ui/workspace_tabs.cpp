// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/workspace_tabs.h"

namespace app {

using namespace ui;

WorkspaceTabs::WorkspaceTabs(TabsDelegate* tabsDelegate)
  : Tabs(tabsDelegate)
  , m_panel(nullptr)
{
}

WorkspaceTabs::~WorkspaceTabs()
{
}

void WorkspaceTabs::setPanel(WorkspacePanel* panel)
{
  ASSERT(!m_panel);
  m_panel = panel;
}

} // namespace app
