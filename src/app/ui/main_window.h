// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_MAIN_WINDOW_H_INCLUDED
#define APP_UI_MAIN_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "ui/window.h"

#include "main_window.xml.h"

namespace ui {
  class Splitter;
  class TooltipManager;
}

namespace app {

#ifdef ENABLE_UPDATER
  class CheckUpdateDelegate;
#endif

  class BrowserView;
  class ColorBar;
  class ContextBar;
  class DevConsoleView;
  class DocView;
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
    StatusBar* statusBar() { return m_statusBar; }
    WorkspaceTabs* getTabsBar() { return m_tabsBar; }
    Timeline* getTimeline() { return m_timeline; }
    Workspace* getWorkspace() { return m_workspace; }
    PreviewEditorWindow* getPreviewEditor() { return m_previewEditor; }
#ifdef ENABLE_UPDATER
    CheckUpdateDelegate* getCheckUpdateDelegate();
#endif
#if ENABLE_SENTRY
    void updateConsentCheckbox();
#endif

    void initialize();
    void start();
    void showNotification(INotificationDelegate* del);
    void showHomeOnOpen();
    void showHome();
    void showDefaultStatusBar();
    void showDevConsole();
    void showBrowser(const std::string& filename,
                     const std::string& section = std::string());
    bool isHomeSelected() const;

    Mode getMode() const { return m_mode; }
    void setMode(Mode mode);

    bool getTimelineVisibility() const;
    void setTimelineVisibility(bool visible);
    void popTimeline();

    // When crash::DataRecovery finish to search for sessions, this
    // function is called.
    void dataRecoverySessionsAreReady();

    // TabsDelegate implementation.
    bool isTabModified(Tabs* tabs, TabView* tabView) override;
    bool canCloneTab(Tabs* tabs, TabView* tabView) override;
    void onSelectTab(Tabs* tabs, TabView* tabView) override;
    void onCloseTab(Tabs* tabs, TabView* tabView) override;
    void onCloneTab(Tabs* tabs, TabView* tabView, int pos) override;
    void onContextMenuTab(Tabs* tabs, TabView* tabView) override;
    void onTabsContainerDoubleClicked(Tabs* tabs) override;
    void onMouseOverTab(Tabs* tabs, TabView* tabView) override;
    void onMouseLeaveTab() override;
    DropViewPreviewResult onFloatingTab(Tabs* tabs,
                                        TabView* tabView,
                                        const gfx::Point& screenPos) override;
    void onDockingTab(Tabs* tabs, TabView* tabView) override;
    DropTabResult onDropTab(Tabs* tabs,
                            TabView* tabView,
                            const gfx::Point& screenPos,
                            const bool clone) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onSaveLayout(ui::SaveLayoutEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onActiveViewChange();
    void onLanguageChange();

  private:
    DocView* getDocView();
    HomeView* getHomeView();
    void configureWorkspaceLayout();

    ui::TooltipManager* m_tooltipManager;
    MainMenuBar* m_menuBar;
    StatusBar* m_statusBar;
    ColorBar* m_colorBar;
    ContextBar* m_contextBar;
    ui::Widget* m_toolBar;
    WorkspaceTabs* m_tabsBar;
    Mode m_mode;
    Timeline* m_timeline;
    Workspace* m_workspace;
    PreviewEditorWindow* m_previewEditor;
    HomeView* m_homeView;
    Notifications* m_notifications;
    INotificationDelegate* m_scalePanic;
    BrowserView* m_browserView;
#ifdef ENABLE_SCRIPTING
    DevConsoleView* m_devConsoleView;
#endif
  };

}

#endif
