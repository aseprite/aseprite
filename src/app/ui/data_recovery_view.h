// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DATA_RECOVERY_VIEW_H_INCLUDED
#define APP_UI_DATA_RECOVERY_VIEW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"
#include "ui/listbox.h"
#include "ui/view.h"

namespace app {
  namespace crash {
    class DataRecovery;
  }

  class DataRecoveryView : public ui::Box
                         , public TabView
                         , public WorkspaceView {
  public:
    DataRecoveryView(crash::DataRecovery* dataRecovery);
    ~DataRecoveryView();

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    void onWorkspaceViewSelected() override;
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onTabPopup(Workspace* workspace) override;

    // Triggered when the list is empty (because the user deleted all
    // sessions).
    base::Signal0<void> Empty;

  private:
    void fillList();

    crash::DataRecovery* m_dataRecovery;
    ui::View m_view;
    ui::ListBox m_listBox;
  };

} // namespace app

#endif
