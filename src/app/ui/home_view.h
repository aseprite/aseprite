// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_HOME_VIEW_H_INCLUDED
#define APP_UI_HOME_VIEW_H_INCLUDED
#pragma once

#include "app/check_update_delegate.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

#include "home_view.xml.h"

namespace ui {
  class View;
}

namespace app {

  class DataRecoveryView;
  class NewsListBox;
  class RecentFilesListBox;
  class RecentFoldersListBox;

  namespace crash {
    class DataRecovery;
  }

  class HomeView : public app::gen::HomeView
                 , public TabView
                 , public WorkspaceView
#ifdef ENABLE_UPDATER
                 , public CheckUpdateDelegate
#endif
  {
  public:
    HomeView();
    ~HomeView();

    // When crash::DataRecovery finish to search for sessions, this
    // function is called.
    void dataRecoverySessionsAreReady();

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onAfterRemoveView(Workspace* workspace) override;
    void onTabPopup(Workspace* workspace) override;
    void onWorkspaceViewSelected() override;

  protected:
    void onResize(ui::ResizeEvent& ev) override;
#ifdef ENABLE_UPDATER
    // CheckUpdateDelegate impl
    void onCheckingUpdates() override;
    void onUpToDate() override;
    void onNewUpdate(const std::string& url, const std::string& version) override;
#endif

  private:
    void onNewFile();
    void onOpenFile();
    void onRecoverSprites();

    RecentFilesListBox* m_files;
    RecentFoldersListBox* m_folders;
    NewsListBox* m_news;
    crash::DataRecovery* m_dataRecovery;
    DataRecoveryView* m_dataRecoveryView;
  };

} // namespace app

#endif
