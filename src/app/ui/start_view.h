// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_START_VIEW_H_INCLUDED
#define APP_UI_START_VIEW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

namespace ui {
  class View;
}

namespace app {
  class StartView : public ui::Box
                  , public TabView
                  , public WorkspaceView {
  public:
    StartView();
    ~StartView();

    // TabView implementation
    std::string getTabText() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    WorkspaceView* cloneWorkspaceView() override;
    void onClonedFrom(WorkspaceView* from) override;
  };

} // namespace app

#endif
