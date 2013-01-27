/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "config.h"

#include "widgets/main_window.h"

#include "app.h"
#include "app/load_widget.h"
#include "app_menus.h"
#include "commands/commands.h"
#include "modules/editors.h"
#include "ui/splitter.h"
#include "ui/system.h"
#include "ui/view.h"
#include "ui_context.h"
#include "widgets/color_bar.h"
#include "widgets/document_view.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_view.h"
#include "widgets/main_menu_bar.h"
#include "widgets/mini_editor.h"
#include "widgets/status_bar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"
#include "widgets/workspace.h"

using namespace ui;
using namespace widgets;

MainWindow::MainWindow()
  : Window(true, NULL)
  , m_lastSplitterPos(0.0)
  , m_advancedMode(false)
{
  setId("main_window");

  // Load all menus by first time.
  AppMenus::instance()->reload();

  Widget* mainBox = app::load_widget<Widget>("main_window.xml", "main_box");
  addChild(mainBox);

  Widget* box_menubar = findChild("menubar");
  Widget* box_colorbar = findChild("colorbar");
  Widget* box_toolbar = findChild("toolbar");
  Widget* box_statusbar = findChild("statusbar");
  Widget* box_tabsbar = findChild("tabsbar");
  Widget* box_workspace = findChild("workspace");

  m_menuBar = new MainMenuBar();
  m_statusBar = new StatusBar();
  m_colorBar = new ColorBar(box_colorbar->getAlign());
  m_toolBar = new ToolBar();
  m_tabsBar = new Tabs(this);
  m_workspace = new Workspace();
  m_miniEditor = new MiniEditorWindow();
  m_colorBarSplitter = findChildT<Splitter>("colorbarsplitter");

  // configure all widgets to expansives
  m_menuBar->setExpansive(true);
  m_statusBar->setExpansive(true);
  m_colorBar->setExpansive(true);
  m_toolBar->setExpansive(true);
  m_tabsBar->setExpansive(true);
  m_workspace->setExpansive(true);

  // Setup the menus
  m_menuBar->setMenu(AppMenus::instance()->getRootMenu());

  // Start text of status bar
  app_default_statusbar_message();

  // Add the widgets in the boxes
  if (box_menubar) box_menubar->addChild(m_menuBar);
  if (box_colorbar) box_colorbar->addChild(m_colorBar);
  if (box_toolbar) box_toolbar->addChild(m_toolBar);
  if (box_statusbar) box_statusbar->addChild(m_statusBar);
  if (box_tabsbar) box_tabsbar->addChild(m_tabsBar);
  if (box_workspace) box_workspace->addChild(m_workspace);

  // Prepare the window
  remapWindow();

  AppMenus::instance()->rebuildRecentList();
}

MainWindow::~MainWindow()
{
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

void MainWindow::setAdvancedMode(bool advanced)
{
  // Check if we already are in the given mode.
  if (m_advancedMode == advanced)
    return;

  m_advancedMode = advanced;

  if (m_advancedMode) {
    m_lastSplitterPos = m_colorBarSplitter->getPosition();
    m_colorBarSplitter->setPosition(0.0);
  }
  else if (m_colorBarSplitter->getPosition() == 0.0)
    m_colorBarSplitter->setPosition(m_lastSplitterPos);

  m_menuBar->setVisible(!advanced);
  m_tabsBar->setVisible(!advanced);
  m_toolBar->setVisible(!advanced);
  m_statusBar->setVisible(!advanced);

  layout();
}

void MainWindow::onSaveLayout(SaveLayoutEvent& ev)
{
  Window::onSaveLayout(ev);

  // Restore splitter position if we are in advanced mode, so we save
  // the original splitter position in the layout.
  if (m_colorBarSplitter->getPosition() == 0.0)
    m_colorBarSplitter->setPosition(m_lastSplitterPos);
}

void MainWindow::clickTab(Tabs* tabs, TabView* tabView, int button)
{
  if (!tabView)
    return;

  DocumentView* docView = static_cast<DocumentView*>(tabView);
  Document* document = docView->getDocument();

  UIContext* context = UIContext::instance();
  context->setActiveView(docView);
  context->updateFlags();

  // Right-button: popup-menu
  if (button & 2) {
    Menu* popup_menu = AppMenus::instance()->getDocumentTabPopupMenu();
    if (popup_menu != NULL) {
      popup_menu->showPopup(jmouse_x(0), jmouse_y(0));
    }
  }
  // Middle-button: close the sprite
  else if (button & 4) {
    Command* close_file_cmd =
      CommandsModule::instance()->getCommandByName(CommandId::CloseFile);

    context->executeCommand(close_file_cmd, NULL);
  }
}

void MainWindow::mouseOverTab(Tabs* tabs, TabView* tabView)
{
  // Note: tabView can be NULL
  if (tabView) {
    DocumentView* docView = static_cast<DocumentView*>(tabView);
    Document* document = docView->getDocument();
    m_statusBar->setStatusText(250, "%s", static_cast<const char*>(document->getFilename()));
  }
  else {
    m_statusBar->clearText();
  }
}
