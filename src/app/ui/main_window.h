// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_MAIN_WINDOW_H_INCLUDED
#define APP_UI_MAIN_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "obs/connection.h"
#include "ui/window.h"

#include <memory>

namespace ui {
class Splitter;
class TooltipManager;
} // namespace ui

namespace app {

#ifdef ENABLE_UPDATER
class CheckUpdateDelegate;
#endif

class BrowserView;
class ColorBar;
class ContextBar;
class DevConsoleView;
class DocView;
class Dock;
class HomeView;
class INotificationDelegate;
class Layout;
class LayoutSelector;
class MainMenuBar;
class Notifications;
class PreviewEditorWindow;
class StatusBar;
class Timeline;
class ToolBar;
class Workspace;
class WorkspaceTabs;

namespace crash {
class DataRecovery;
}

class MainWindow : public ui::Window,
                   public TabsDelegate {
public:
  enum Mode { NormalMode, ContextBarAndTimelineMode, EditorOnlyMode };

  MainWindow();
  ~MainWindow();

  // TODO refactor: remove the get prefix from these functions
  MainMenuBar* getMenuBar() { return m_menuBar.get(); }
  LayoutSelector* layoutSelector() { return m_layoutSelector.get(); }
  ContextBar* getContextBar() { return m_contextBar.get(); }
  StatusBar* statusBar() { return m_statusBar.get(); }
  WorkspaceTabs* getTabsBar() { return m_tabsBar.get(); }
  Timeline* getTimeline() { return m_timeline.get(); }
  Workspace* getWorkspace() { return m_workspace.get(); }
  ColorBar* colorBar() { return m_colorBar.get(); }
  ToolBar* toolBar() { return m_toolBar.get(); }
  PreviewEditorWindow* getPreviewEditor() { return m_previewEditor.get(); }
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
  void showBrowser(const std::string& filename, const std::string& section = std::string());
  bool isHomeSelected() const;

  Mode getMode() const { return m_mode; }
  void setMode(Mode mode);

  bool getTimelineVisibility() const;
  void setTimelineVisibility(bool visible);
  void popTimeline();

  void setDefaultLayout();
  void setMirroredDefaultLayout();
  void loadUserLayout(const Layout* layout);
  const Dock* customizableDock() const { return m_customizableDock; }

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
  void onResize(ui::ResizeEvent& ev) override;
  void onBeforeViewChange();
  void onActiveViewChange();
  void onLanguageChange();

  void onDrop(ui::DragEvent& e) override;

private:
  DocView* getDocView();
  HomeView* getHomeView();
  void configureWorkspaceLayout();
  void saveTimelineConfiguration();
  void saveColorBarConfiguration();
  void saveActiveLayout();

  ui::TooltipManager* m_tooltipManager;
  Dock* m_dock;
  Dock* m_customizableDock;
  std::unique_ptr<Widget> m_customizableDockPlaceholder;
  std::unique_ptr<MainMenuBar> m_menuBar;
  std::unique_ptr<LayoutSelector> m_layoutSelector;
  std::unique_ptr<StatusBar> m_statusBar;
  std::unique_ptr<ColorBar> m_colorBar;
  std::unique_ptr<ContextBar> m_contextBar;
  std::unique_ptr<ToolBar> m_toolBar;
  std::unique_ptr<WorkspaceTabs> m_tabsBar;
  Mode m_mode;
  std::unique_ptr<Timeline> m_timeline;
  std::unique_ptr<Workspace> m_workspace;
  std::unique_ptr<PreviewEditorWindow> m_previewEditor;
  std::unique_ptr<HomeView> m_homeView;
  std::unique_ptr<Notifications> m_notifications;
  std::unique_ptr<INotificationDelegate> m_scalePanic;
  std::unique_ptr<BrowserView> m_browserView;
#ifdef ENABLE_SCRIPTING
  std::unique_ptr<DevConsoleView> m_devConsoleView;
#endif
  obs::scoped_connection m_timelineResizeConn;
  obs::scoped_connection m_colorBarResizeConn;
  obs::scoped_connection m_saveDockLayoutConn;
};

} // namespace app

#endif
