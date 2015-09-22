// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_MAIN_WINDOW_H_INCLUDED
#define APP_UI_MAIN_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "ui/window.h"

#include "main_window.xml.h"

namespace ui {
  class Splitter;
}

namespace app {

#ifdef ENABLE_UPDATER
  class CheckUpdateDelegate;
#endif

  class ColorBar;
  class ContextBar;
  class DevConsoleView;
  class DocumentView;
  class HomeView;
  class INotificationDelegate;
  class MainMenuBar;
  class Notifications;
  class PreviewEditorWindow;
  class StatusBar;
  class Timeline;
  class Workspace;
  class WorkspaceTabs;

  namespace crash {
    class DataRecovery;
  }

  class MainWindow : public app::gen::MainWindow
                   , public TabsDelegate {
  public:
    enum Mode {
      NormalMode,
      ContextBarAndTimelineMode,
      EditorOnlyMode
    };

    MainWindow();
    ~MainWindow();

    MainMenuBar* getMenuBar() { return m_menuBar; }
    ContextBar* getContextBar() { return m_contextBar; }
    WorkspaceTabs* getTabsBar() { return m_tabsBar; }
    Timeline* getTimeline() { return m_timeline; }
    Workspace* getWorkspace() { return m_workspace; }
    PreviewEditorWindow* getPreviewEditor() { return m_previewEditor; }
#ifdef ENABLE_UPDATER
    CheckUpdateDelegate* getCheckUpdateDelegate();
#endif

    void start();
    void reloadMenus();
    void showNotification(INotificationDelegate* del);
    void showHomeOnOpen();
    void showHome();
    bool isHomeSelected();
    void showDevConsole();

    Mode getMode() const { return m_mode; }
    void setMode(Mode mode);

    bool getTimelineVisibility() const;
    void setTimelineVisibility(bool visible);
    void popTimeline();

    void showDataRecovery(crash::DataRecovery* dataRecovery);

    // TabsDelegate implementation.
    bool isTabModified(Tabs* tabs, TabView* tabView) override;
    bool canCloneTab(Tabs* tabs, TabView* tabView) override;
    void onSelectTab(Tabs* tabs, TabView* tabView) override;
    void onCloseTab(Tabs* tabs, TabView* tabView) override;
    void onCloneTab(Tabs* tabs, TabView* tabView, int pos) override;
    void onContextMenuTab(Tabs* tabs, TabView* tabView) override;
    void onMouseOverTab(Tabs* tabs, TabView* tabView) override;
    DropViewPreviewResult onFloatingTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos) override;
    void onDockingTab(Tabs* tabs, TabView* tabView) override;
    DropTabResult onDropTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos, bool clone) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSaveLayout(ui::SaveLayoutEvent& ev) override;
    void onActiveViewChange();

  private:
    DocumentView* getDocView();
    HomeView* getHomeView();
    void configureWorkspaceLayout();

    MainMenuBar* m_menuBar;
    ContextBar* m_contextBar;
    StatusBar* m_statusBar;
    ColorBar* m_colorBar;
    ui::Widget* m_toolBar;
    WorkspaceTabs* m_tabsBar;
    Mode m_mode;
    Timeline* m_timeline;
    Workspace* m_workspace;
    PreviewEditorWindow* m_previewEditor;
    HomeView* m_homeView;
    DevConsoleView* m_devConsoleView;
    Notifications* m_notifications;
  };

}

#endif
