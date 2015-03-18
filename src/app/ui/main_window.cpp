// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/main_window.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/commands.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/devconsole_view.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/home_view.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/notifications.h"
#include "app/ui/preview_editor.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/tabs.h"
#include "app/ui/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "ui/message.h"
#include "ui/splitter.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;

MainWindow::MainWindow()
  : m_mode(NormalMode)
  , m_homeView(nullptr)
  , m_devConsoleView(nullptr)
{
  // Load all menus by first time.
  AppMenus::instance()->reload();

  m_menuBar = new MainMenuBar();
  m_notifications = new Notifications();
  m_contextBar = new ContextBar();
  m_statusBar = new StatusBar();
  m_colorBar = new ColorBar(colorBarPlaceholder()->getAlign());
  m_toolBar = new ToolBar();
  m_tabsBar = new Tabs(this);
  m_workspace = new Workspace();
  m_workspace->ActiveViewChanged.connect(&MainWindow::onActiveViewChange, this);
  m_previewEditor = new PreviewEditorWindow();
  m_timeline = new Timeline();

  // configure all widgets to expansives
  m_menuBar->setExpansive(true);
  m_contextBar->setExpansive(true);
  m_contextBar->setVisible(false);
  m_statusBar->setExpansive(true);
  m_colorBar->setExpansive(true);
  m_toolBar->setExpansive(true);
  m_tabsBar->setExpansive(true);
  m_timeline->setExpansive(true);
  m_workspace->setExpansive(true);
  m_notifications->setVisible(false);

  // Setup the menus
  m_menuBar->setMenu(AppMenus::instance()->getRootMenu());

  // Add the widgets in the boxes
  menuBarPlaceholder()->addChild(m_menuBar);
  menuBarPlaceholder()->addChild(m_notifications);
  contextBarPlaceholder()->addChild(m_contextBar);
  colorBarPlaceholder()->addChild(m_colorBar);
  toolBarPlaceholder()->addChild(m_toolBar);
  statusBarPlaceholder()->addChild(m_statusBar);
  tabsPlaceholder()->addChild(m_tabsBar);
  workspacePlaceholder()->addChild(m_workspace);
  timelinePlaceholder()->addChild(m_timeline);

  // Default splitter positions
  colorBarSplitter()->setPosition(m_colorBar->getPreferredSize().w);
  timelineSplitter()->setPosition(75);

  // Prepare the window
  remapWindow();

  AppMenus::instance()->rebuildRecentList();
}

MainWindow::~MainWindow()
{
  if (m_devConsoleView) {
    if (m_devConsoleView->getParent())
      m_workspace->removeView(m_devConsoleView);
    delete m_devConsoleView;
  }
  if (m_homeView) {
    if (m_homeView->getParent())
      m_workspace->removeView(m_homeView);
    delete m_homeView;
  }
  delete m_contextBar;
  delete m_previewEditor;

  // Destroy the workspace first so ~Editor can dettach slots from
  // ColorBar. TODO this is a terrible hack for slot/signal stuff,
  // connections should be handle in a better/safer way.
  delete m_workspace;

  // Remove the root-menu from the menu-bar (because the rootmenu
  // module should destroy it).
  m_menuBar->setMenu(NULL);
}

DocumentView* MainWindow::getDocView()
{
  return dynamic_cast<DocumentView*>(m_workspace->activeView());
}

HomeView* MainWindow::getHomeView()
{
  if (!m_homeView)
    m_homeView = new HomeView;
  return m_homeView;
}

#ifdef ENABLE_UPDATER
CheckUpdateDelegate* MainWindow::getCheckUpdateDelegate()
{
  return getHomeView();
}
#endif

void MainWindow::reloadMenus()
{
  m_menuBar->reload();

  layout();
  invalidate();
}

void MainWindow::showNotification(INotificationDelegate* del)
{
  m_notifications->addLink(del);
  m_notifications->setVisible(true);
  m_notifications->getParent()->layout();
}

void MainWindow::showHome()
{
  if (!getHomeView()->getParent()) {
    TabView* selectedTab = m_tabsBar->getSelectedTab();

    // Show "Home" tab in the first position, and select it only if
    // there is no other view selected.
    m_workspace->addView(m_homeView, 0);
    if (selectedTab)
      m_tabsBar->selectTab(selectedTab);
    else
      m_tabsBar->selectTab(m_homeView);
  }
}

void MainWindow::showDevConsole()
{
  if (!m_devConsoleView)
    m_devConsoleView = new DevConsoleView;

  if (!m_devConsoleView->getParent()) {
    m_workspace->addView(m_devConsoleView);
    m_tabsBar->selectTab(m_devConsoleView);
  }
}

void MainWindow::setMode(Mode mode)
{
  // Check if we already are in the given mode.
  if (m_mode == mode)
    return;

  m_mode = mode;
  configureWorkspaceLayout();
}

bool MainWindow::getTimelineVisibility() const
{
  return App::instance()->preferences().general.visibleTimeline();
}

void MainWindow::setTimelineVisibility(bool visible)
{
  App::instance()->preferences().general.visibleTimeline(visible);

  configureWorkspaceLayout();
}

void MainWindow::popTimeline()
{
  Preferences& preferences = App::instance()->preferences();

  if (!preferences.general.autoshowTimeline())
    return;

  if (!getTimelineVisibility())
    setTimelineVisibility(true);
}

bool MainWindow::onProcessMessage(ui::Message* msg)
{
  if (msg->type() == kOpenMessage)
    showHome();

  return Window::onProcessMessage(msg);
}

void MainWindow::onSaveLayout(SaveLayoutEvent& ev)
{
  Window::onSaveLayout(ev);
}

// When the active view is changed from methods like
// Workspace::splitView(), this function is called, and we have to
// inform to the UIContext that the current view has changed.
void MainWindow::onActiveViewChange()
{
  if (DocumentView* docView = getDocView())
    UIContext::instance()->setActiveView(docView);
  else
    UIContext::instance()->setActiveView(nullptr);

  configureWorkspaceLayout();
}

void MainWindow::onSelectTab(Tabs* tabs, TabView* tabView)
{
  if (!tabView)
    return;

  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  if (m_workspace->activeView() != view)
    m_workspace->setActiveView(view);
}

void MainWindow::onCloseTab(Tabs* tabs, TabView* tabView)
{
  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  ASSERT(view);
  if (view)
    m_workspace->closeView(view);
}

void MainWindow::onContextMenuTab(Tabs* tabs, TabView* tabView)
{
  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  ASSERT(view);
  if (view)
    view->onTabPopup(m_workspace);
}

void MainWindow::onMouseOverTab(Tabs* tabs, TabView* tabView)
{
  // Note: tabView can be NULL
  if (DocumentView* docView = dynamic_cast<DocumentView*>(tabView)) {
    Document* document = docView->getDocument();
    m_statusBar->setStatusText(250, "%s",
      document->filename().c_str());
  }
  else {
    m_statusBar->clearText();
  }
}

bool MainWindow::onIsModified(Tabs* tabs, TabView* tabView)
{
  if (DocumentView* docView = dynamic_cast<DocumentView*>(tabView)) {
    Document* document = docView->getDocument();
    return document->isModified();
  }
  else {
    return false;
  }
}

void MainWindow::configureWorkspaceLayout()
{
  bool normal = (m_mode == NormalMode);
  bool isDoc = (getDocView() != nullptr);

  m_menuBar->setVisible(normal);
  m_tabsBar->setVisible(normal);
  colorBarPlaceholder()->setVisible(normal && isDoc);
  m_toolBar->setVisible(normal && isDoc);
  m_statusBar->setVisible(normal);
  m_contextBar->setVisible(
    isDoc &&
    (m_mode == NormalMode ||
     m_mode == ContextBarAndTimelineMode));
  timelinePlaceholder()->setVisible(
    isDoc &&
    (m_mode == NormalMode ||
     m_mode == ContextBarAndTimelineMode) &&
    App::instance()->preferences().general.visibleTimeline());

  if (m_contextBar->isVisible()) {
    m_contextBar->updateFromTool(
      UIContext::instance()->settings()->getCurrentTool());
  }

  layout();
}

} // namespace app
