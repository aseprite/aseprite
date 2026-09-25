// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_HOME_VIEW_H_INCLUDED
#define APP_UI_HOME_VIEW_H_INCLUDED
#pragma once

#include "app/check_update_delegate.h"
#include "app/ui/input_chain_element.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

#include "home_view.xml.h"

namespace ui {
class View;
}

namespace app {

class NewsListBox;
class RecentGrid;

class HomeView : public app::gen::HomeView,
                 public TabView,
                 public WorkspaceView,
                 public app::InputChainElement
#ifdef ENABLE_UPDATER
  ,
                 public CheckUpdateDelegate
#endif
{
public:
  HomeView();

#if ENABLE_SENTRY
  void updateConsentCheckbox();
#endif

  // TabView implementation
  std::string getTabText() override;
  TabIcon getTabIcon() override;
  gfx::Color getTabColor() override;

  // WorkspaceView implementation
  ui::Widget* getContentWidget() override { return this; }
  bool onCloseView(Workspace* workspace, bool quitting) override;
  void onTabPopup(Workspace* workspace) override;
  void onWorkspaceViewSelected() override;
  InputChainElement* onGetInputChainElement() override { return this; }

  // InputChainElement impl
  void onNewInputPriority(InputChainElement* element, const ui::Message* msg) override;
  bool onCanCut(Context* ctx) override;
  bool onCanCopy(Context* ctx) override;
  bool onCanPaste(Context* ctx) override;
  bool onCanClear(Context* ctx) override;
  bool onCut(Context* ctx) override;
  bool onCopy(Context* ctx) override;
  bool onPaste(Context* ctx, const gfx::Point* position) override;
  bool onClear(Context* ctx) override;
  void onCancel(Context* ctx) override;

protected:
  bool onProcessMessage(ui::Message* msg) override;
#ifdef ENABLE_NEWS
  void setShowNews(bool showNews);
#endif
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

  bool m_showNews;
  obs::scoped_connection m_recentFilesConn;

  RecentGrid* m_recents;
  NewsListBox* m_news;
};

} // namespace app

#endif
