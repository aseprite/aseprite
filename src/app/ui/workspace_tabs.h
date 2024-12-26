// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_WORKSPACE_TABS_H_INCLUDED
#define APP_UI_WORKSPACE_TABS_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"

namespace app {
class WorkspacePanel;

class WorkspaceTabs : public Tabs {
public:
  ui::WidgetType Type();

  WorkspaceTabs(TabsDelegate* tabsDelegate);
  ~WorkspaceTabs();

  WorkspacePanel* panel() const { return m_panel; }
  void setPanel(WorkspacePanel* panel);

private:
  WorkspacePanel* m_panel;
};

} // namespace app

#endif
