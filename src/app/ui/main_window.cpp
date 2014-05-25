/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/mini_editor.h"
#include "app/ui/notifications.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/start_view.h"
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
  : Window(DesktopWindow)
  , m_lastSplitterPos(0.0)
  , m_lastTimelineSplitterPos(75.0)
  , m_mode(NormalMode)
  , m_startView(NULL)
{
  setId("main_window");

  // Load all menus by first time.
  AppMenus::instance()->reload();

  Widget* mainBox = app::load_widget<Widget>("main_window.xml", "main_box");
  addChild(mainBox);

  Widget* box_menubar = findChild("menubar");
  Widget* box_contextbar = findChild("contextbar");
  Widget* box_colorbar = findChild("colorbar");
  Widget* box_toolbar = findChild("toolbar");
  Widget* box_statusbar = findChild("statusbar");
  Widget* box_tabsbar = findChild("tabsbar");
  Widget* box_workspace = findChild("workspace");
  Widget* box_timeline = findChild("timeline");

  m_menuBar = new MainMenuBar();
  m_notifications = new Notifications();
  m_contextBar = new ContextBar();
  m_statusBar = new StatusBar();
  m_colorBar = new ColorBar(box_colorbar->getAlign());
  m_toolBar = new ToolBar();
  m_tabsBar = new Tabs(this);
  m_workspace = new Workspace();
  m_workspace->ActiveViewChanged.connect(&MainWindow::onActiveViewChange, this);
  m_miniEditor = new MiniEditorWindow();
  m_timeline = new Timeline();
  m_colorBarSplitter = findChildT<Splitter>("colorbarsplitter");
  m_timelineSplitter = findChildT<Splitter>("timelinesplitter");

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
  if (box_menubar) {
    box_menubar->addChild(m_menuBar);
    box_menubar->addChild(m_notifications);
  }
  if (box_contextbar) box_contextbar->addChild(m_contextBar);
  if (box_colorbar) box_colorbar->addChild(m_colorBar);
  if (box_toolbar) box_toolbar->addChild(m_toolBar);
  if (box_statusbar) box_statusbar->addChild(m_statusBar);
  if (box_tabsbar) box_tabsbar->addChild(m_tabsBar);
  if (box_workspace) box_workspace->addChild(m_workspace);
  if (box_timeline) box_timeline->addChild(m_timeline);

  // Default layout of widgets
  m_colorBarSplitter->setPosition(m_colorBar->getPreferredSize().w);

  // Prepare the window
  remapWindow();

  AppMenus::instance()->rebuildRecentList();
}

MainWindow::~MainWindow()
{
  if (m_startView) {
    m_workspace->removeView(m_startView);
    delete m_startView;
  }
  delete m_contextBar;
  delete m_miniEditor;

  // Destroy the workspace first so ~Editor can dettach slots from
  // ColorBar. TODO this is a terrible hack for slot/signal stuff,
  // connections should be handle in a better/safer way.
  delete m_workspace;

  // Remove the root-menu from the menu-bar (because the rootmenu
  // module should destroy it).
  m_menuBar->setMenu(NULL);
}

void MainWindow::reloadMenus()
{
  m_menuBar->reload();

  layout();
  invalidate();
}

void MainWindow::showNotification(const char* text, const char* url)
{
  m_notifications->addLink(text, url);
  m_notifications->setVisible(true);
  m_notifications->getParent()->layout();
}

void MainWindow::setMode(Mode mode)
{
  // Check if we already are in the given mode.
  if (m_mode == mode)
    return;

  if (mode == NormalMode) {
    if (m_colorBarSplitter->getPosition() == 0.0)
      m_colorBarSplitter->setPosition(m_lastSplitterPos);
  }
  // If current mode is "normal", we save the splitter position of the
  // color bar in "m_lastSplitterPos" before we hide it.
  else if (m_mode == NormalMode) {
    m_lastSplitterPos = m_colorBarSplitter->getPosition();
    m_colorBarSplitter->setPosition(0.0);
  }

  m_menuBar->setVisible(mode == NormalMode);
  m_tabsBar->setVisible(mode == NormalMode);
  m_toolBar->setVisible(mode == NormalMode);
  m_statusBar->setVisible(mode == NormalMode);
  m_contextBar->setVisible(
    mode == NormalMode ||
    mode == ContextBarAndTimelineMode);
  setTimelineVisibility(
    mode == NormalMode ||
    mode == ContextBarAndTimelineMode);

  m_mode = mode;
  layout();
}

bool MainWindow::getTimelineVisibility() const
{
  return m_timelineSplitter->getPosition() < 100.0;
}

void MainWindow::setTimelineVisibility(bool visible)
{
  if (visible) {
    if (m_timelineSplitter->getPosition() >= 100.0)
      m_timelineSplitter->setPosition(m_lastTimelineSplitterPos);
  }
  else {
    if (m_timelineSplitter->getPosition() < 100.0) {
      m_lastTimelineSplitterPos = m_timelineSplitter->getPosition();
      m_timelineSplitter->setPosition(100.0);
    }
  }
  layout();
}

void MainWindow::popTimeline()
{
  if (!get_config_bool("Options", "AutoShowTimeline", true))
    return;

  if (!getTimelineVisibility())
    setTimelineVisibility(true);
}

bool MainWindow::onProcessMessage(ui::Message* msg)
{
#if 0                           // TODO Enable start view
  if (msg->type() == kOpenMessage) {
    m_startView = new StartView;
    m_workspace->addView(m_startView);
    m_tabsBar->selectTab(m_startView);
  }
#endif

  return Window::onProcessMessage(msg);
}

void MainWindow::onSaveLayout(SaveLayoutEvent& ev)
{
  Window::onSaveLayout(ev);

  // Restore splitter position if we are in advanced mode, so we save
  // the original splitter position in the layout.
  if (m_colorBarSplitter->getPosition() == 0.0)
    m_colorBarSplitter->setPosition(m_lastSplitterPos);
}

// When the active view is changed from methods like
// Workspace::splitView(), this function is called, and we have to
// inform to the UIContext that the current view has changed.
void MainWindow::onActiveViewChange()
{
  if (DocumentView* docView = dynamic_cast<DocumentView*>(m_workspace->getActiveView())) {
    UIContext::instance()->setActiveView(docView);

    m_contextBar->updateFromTool(UIContext::instance()
      ->getSettings()->getCurrentTool());

    if (m_mode != EditorOnlyMode)
      m_contextBar->setVisible(true);
  }
  else {
    UIContext::instance()->setActiveView(NULL);

    if (m_mode != EditorOnlyMode)
      m_contextBar->setVisible(false);
  }
  layout();
}

void MainWindow::clickTab(Tabs* tabs, TabView* tabView, ui::MouseButtons buttons)
{
  if (!tabView)
    return;

  WorkspaceView* workspaceView = dynamic_cast<WorkspaceView*>(tabView);
  if (m_workspace->getActiveView() != workspaceView)
    m_workspace->setActiveView(workspaceView);

  DocumentView* docView = dynamic_cast<DocumentView*>(workspaceView);
  if (!docView)
    return;

  UIContext* context = UIContext::instance();
  context->setActiveView(docView);
  context->updateFlags();

  // Right-button: popup-menu
  if (buttons & kButtonRight) {
    Menu* popup_menu = AppMenus::instance()->getDocumentTabPopupMenu();
    if (popup_menu != NULL) {
      popup_menu->showPopup(jmouse_x(0), jmouse_y(0));
    }
  }
  // Middle-button: close the sprite
  else if (buttons & kButtonMiddle) {
    Command* close_file_cmd =
      CommandsModule::instance()->getCommandByName(CommandId::CloseFile);

    context->executeCommand(close_file_cmd, NULL);
  }
}

void MainWindow::mouseOverTab(Tabs* tabs, TabView* tabView)
{
  // Note: tabView can be NULL
  if (DocumentView* docView = dynamic_cast<DocumentView*>(tabView)) {
    Document* document = docView->getDocument();
    m_statusBar->setStatusText(250, "%s",
                               document->getFilename().c_str());
  }
  else {
    m_statusBar->clearText();
  }
}

} // namespace app
