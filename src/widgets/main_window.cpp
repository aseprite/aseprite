/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "app_menus.h"
#include "app/load_widget.h"
#include "commands/commands.h"
#include "modules/editors.h"
#include "ui/system.h"
#include "ui/view.h"
#include "ui_context.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_view.h"
#include "widgets/main_menu_bar.h"
#include "widgets/status_bar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

using namespace ui;

class AppTabsDelegate : public TabsDelegate
{
public:
  void clickTab(Tabs* tabs, void* data, int button);
  void mouseOverTab(Tabs* tabs, void* data);
};

MainWindow::MainWindow()
  : Window(true, NULL)
{
  // Load all menus by first time.
  AppMenus::instance()->reload();

  Widget* mainBox = app::load_widget<Widget>("main_window.xml", "main_box");
  addChild(mainBox);

  box_editors = findChild("editor"); // WARNING / TODO this is a
                                     // global variable defined in
                                     // legacy "editors" modules

  Widget* box_menubar = findChild("menubar");
  Widget* box_colorbar = findChild("colorbar");
  Widget* box_toolbar = findChild("toolbar");
  Widget* box_statusbar = findChild("statusbar");
  Widget* box_tabsbar = findChild("tabsbar");

  m_menuBar = new MainMenuBar();
  m_statusBar = new StatusBar();
  m_colorBar = new ColorBar(box_colorbar->getAlign());
  m_toolBar = new ToolBar();
  m_tabsBar = new Tabs(m_tabsDelegate = new AppTabsDelegate());

  // configure all widgets to expansives
  m_menuBar->setExpansive(true);
  m_statusBar->setExpansive(true);
  m_colorBar->setExpansive(true);
  m_toolBar->setExpansive(true);
  m_tabsBar->setExpansive(true);

  // Setup the menus
  m_menuBar->setMenu(AppMenus::instance()->getRootMenu());

  /* start text of status bar */
  app_default_statusbar_message();

  /* add the widgets in the boxes */
  if (box_menubar) box_menubar->addChild(m_menuBar);
  if (box_colorbar) box_colorbar->addChild(m_colorBar);
  if (box_toolbar) box_toolbar->addChild(m_toolBar);
  if (box_statusbar) box_statusbar->addChild(m_statusBar);
  if (box_tabsbar) box_tabsbar->addChild(m_tabsBar);

  // Prepare the window
  remap_window();

  // Initialize editors.
  init_module_editors();

  // Create the list of tabs
  app_rebuild_documents_tabs();
  AppMenus::instance()->rebuildRecentList();
}

MainWindow::~MainWindow()
{
  delete m_tabsDelegate;

  // Remove the root-menu from the menu-bar (because the rootmenu
  // module should destroy it).
  m_menuBar->setMenu(NULL);

  // Delete all editors first because they used signals from other
  // widgets (e.g. color bar).
  delete box_editors;

  // Destroy mini-editor.
  exit_module_editors();
}

void MainWindow::reloadMenus()
{
  m_menuBar->reload();

  remap_window();
  invalidate();
}

void MainWindow::createFirstEditor()
{
  View* view = new EditorView(EditorView::CurrentEditorMode);
  Editor* editor = create_new_editor();

  // Prepare the first editor
  view->attachToView(editor);
  view->setExpansive(true);

  if (box_editors)
    box_editors->addChild(view);

  // Set current editor
  set_current_editor(editor);   // TODO remove this line from here
}

//////////////////////////////////////////////////////////////////////
// AppTabsDelegate

void AppTabsDelegate::clickTab(Tabs* tabs, void* data, int button)
{
  Document* document = reinterpret_cast<Document*>(data);

  // put as current sprite
  set_document_in_more_reliable_editor(document);

  if (document) {
    Context* context = UIContext::instance();
    context->updateFlags();

    // right-button: popup-menu
    if (button & 2) {
      Menu* popup_menu = AppMenus::instance()->getDocumentTabPopupMenu();
      if (popup_menu != NULL) {
        popup_menu->showPopup(jmouse_x(0), jmouse_y(0));
      }
    }
    // middle-button: close the sprite
    else if (button & 4) {
      Command* close_file_cmd =
        CommandsModule::instance()->getCommandByName(CommandId::CloseFile);

      context->executeCommand(close_file_cmd, NULL);
    }
  }
}

void AppTabsDelegate::mouseOverTab(Tabs* tabs, void* data)
{
  // Note: data can be NULL
  Document* document = (Document*)data;

  if (data) {
    StatusBar::instance()->setStatusText(250, "%s",
                                         static_cast<const char*>(document->getFilename()));
  }
  else {
    StatusBar::instance()->clearText();
  }
}
