// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_BROWSER_VIEW_H_INCLUDED
#define APP_UI_BROWSER_VIEW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/view.h"

namespace app {
  class BrowserView : public ui::Widget
                    , public TabView
                    , public WorkspaceView {
  public:
    BrowserView();
    ~BrowserView();

    void loadFile(const std::string& file);

    // TabView implementation
    std::string getTabText() override;
    TabIcon getTabIcon() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    bool canCloneWorkspaceView() override { return true; }
    WorkspaceView* cloneWorkspaceView() override;
    void onWorkspaceViewSelected() override;
    bool onCloseView(Workspace* workspace, bool quitting) override;
    void onTabPopup(Workspace* workspace) override;

  private:
    class CMarkBox;

    ui::View m_view;
    CMarkBox* m_textBox;
    std::string m_title;
    std::string m_filename;
  };

} // namespace app

#endif
