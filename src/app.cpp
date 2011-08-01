/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "app.h"

#include "app/check_update.h"
#include "app/color_utils.h"
#include "base/exception.h"
#include "base/unique_ptr.h"
#include "check_args.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "drop_files.h"
#include "file/file.h"
#include "file/file_formats_manager.h"
#include "file_system.h"
#include "gui/gui.h"
#include "gui/intern.h"
#include "gui_xml.h"
#include "ini_file.h"
#include "log.h"
#include "modules.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/rootmenu.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "recent_files.h"
#include "tools/tool_box.h"
#include "ui_context.h"
#include "util/boundary.h"
#include "util/render.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_view.h"
#include "widgets/menuitem2.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

#include <allegro.h>
/* #include <allegro/internal/aintern.h> */
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef ALLEGRO_WINDOWS
  #include <winalleg.h>
#endif

/**
 * ASE modules.
 */
class App::Modules
{
public:
  // ASE Modules
  FileSystemModule m_file_system_module;
  tools::ToolBox m_toolbox;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
};

class AppTabsDelegate : public TabsDelegate
{
public:
  void clickTab(Tabs* tabs, void* data, int button);
  void mouseOverTab(Tabs* tabs, void* data);
};

App* App::m_instance = NULL;

static Frame* top_window = NULL;      /* top level window (the desktop) */
static Widget* box_menubar = NULL;    /* box where the menu bar is */
static Widget* box_colorbar = NULL;   /* box where the color bar is */
static Widget* box_toolbar = NULL;    /* box where the tools bar is */
static Widget* box_statusbar = NULL;  /* box where the status bar is */
static Widget* box_tabsbar = NULL;    /* box where the tabs bar is */
static MenuBar* menubar = NULL;	      /* the menu bar widget */
static StatusBar* statusbar = NULL;   /* the status bar widget */
static ColorBar* colorbar = NULL;     /* the color bar widget */
static Widget* toolbar = NULL;	      /* the tool bar widget */
static Tabs* tabsbar = NULL;	      // The tabs bar widget

static char *palette_filename = NULL;

// Initializes the application loading the modules, setting the
// graphics mode, loading the configuration and resources, etc.
App::App(int argc, char* argv[])
  : m_configModule(NULL)
  , m_checkArgs(NULL)
  , m_loggerModule(NULL)
  , m_modules(NULL)
  , m_legacy(NULL)
  , m_isGui(false)
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  for (int i = 0; i < argc; ++i)
    m_args.push_back(argv[i]);

  m_configModule = new ConfigModule();
  m_checkArgs = new CheckArgs(m_args);
  m_loggerModule = new LoggerModule(m_checkArgs->isVerbose());
  m_modules = new Modules();
  m_isGui = !(m_checkArgs->isConsoleOnly());
  m_legacy = new LegacyModules(isGui() ? REQUIRE_INTERFACE: 0);
 
  // Register well-known image file types.
  FileFormatsManager::instance().registerAllFormats();

  // init editor cursor
  Editor::editor_cursor_init();

  // Load RenderEngine configuration
  RenderEngine::loadConfig();

  /* custom default palette? */
  if (palette_filename) {
    PRINTF("Loading custom palette file: %s\n", palette_filename);

    UniquePtr<Palette> pal(Palette::load(palette_filename));
    if (pal.get() == NULL)
      throw base::Exception("Error loading default palette from: %s",
			    static_cast<const char*>(palette_filename));

    set_default_palette(pal.get());
  }

  /* set system palette to the default one */
  set_current_palette(NULL, true);
}



int App::run()
{
#ifdef ENABLE_UPDATER
  app::CheckUpdateThreadLauncher checkUpdate;
#endif

  // Initialize GUI interface
  if (isGui()) {
    View* view;
    Editor* editor;

    PRINTF("GUI mode\n");

    // Setup the GUI screen
    jmouse_set_cursor(JI_CURSOR_NORMAL);
    jmanager_refresh_screen();

    // Load main window
    top_window = static_cast<Frame*>(load_widget("main_window.xml", "main_window"));

    box_menubar = top_window->findChild("menubar");
    box_editors = top_window->findChild("editor");
    box_colorbar = top_window->findChild("colorbar");
    box_toolbar = top_window->findChild("toolbar");
    box_statusbar = top_window->findChild("statusbar");
    box_tabsbar = top_window->findChild("tabsbar");

    menubar = new MenuBar();
    statusbar = new StatusBar();
    colorbar = new ColorBar(box_colorbar->getAlign());
    toolbar = toolbar_new();
    tabsbar = new Tabs(m_tabsDelegate = new AppTabsDelegate());
    view = new EditorView(EditorView::CurrentEditorMode);
    editor = create_new_editor();

    // configure all widgets to expansives
    jwidget_expansive(menubar, true);
    jwidget_expansive(statusbar, true);
    jwidget_expansive(colorbar, true);
    jwidget_expansive(toolbar, true);
    jwidget_expansive(tabsbar, true);
    jwidget_expansive(view, true);

    /* prepare the first editor */
    view->attachToView(editor);

    /* setup the menus */
    menubar->setMenu(get_root_menu());

    /* start text of status bar */
    app_default_statusbar_message();

    /* add the widgets in the boxes */
    if (box_menubar) box_menubar->addChild(menubar);
    if (box_editors) box_editors->addChild(view);
    if (box_colorbar) box_colorbar->addChild(colorbar);
    if (box_toolbar) box_toolbar->addChild(toolbar);
    if (box_statusbar) box_statusbar->addChild(statusbar);
    if (box_tabsbar) box_tabsbar->addChild(tabsbar);

    /* prepare the window */
    top_window->remap_window();

    // Create the list of tabs
    app_rebuild_documents_tabs();
    app_rebuild_recent_list();

    /* set current editor */
    set_current_editor(editor);

    /* open the window */
    top_window->open_window();

    /* refresh the screen */
    jmanager_refresh_screen();
  }

  /* set background mode for non-GUI modes */
/*   if (!(ase_mode & MODE_GUI)) */
/*     set_display_switch_mode(SWITCH_BACKAMNESIA); */
    set_display_switch_mode(SWITCH_BACKGROUND);

    // procress options
  PRINTF("Processing options...\n");
  
  ASSERT(m_checkArgs != NULL);
  {
    Console console;
    for (CheckArgs::iterator
	   it  = m_checkArgs->begin();
         it != m_checkArgs->end(); ++it) {
      CheckArgs::Option* option = *it;

      switch (option->type()) {

	case CheckArgs::Option::OpenSprite: {
	  // Load the sprite
	  Document* document = load_document(option->data().c_str());
	  if (!document) {
	    if (!isGui())
	      console.printf("Error loading file \"%s\"\n", option->data().c_str());
	  }
	  else {
	    // Mount and select the sprite
	    UIContext* context = UIContext::instance();
	    context->addDocument(document);
	    context->setActiveDocument(document);

	    if (isGui()) {
	      // Show it
	      set_document_in_more_reliable_editor(context->getFirstDocument());

	      // Recent file
	      getRecentFiles()->addRecentFile(option->data().c_str());
	    }
	  }
	  break;
	}
      }
    }
    delete m_checkArgs;
    m_checkArgs = NULL;
  }

  // Run the GUI
  if (isGui()) {
    // Support to drop files from Windows explorer
    install_drop_files();

#ifdef ENABLE_UPDATER
    // Launch the thread to check for updates.
    checkUpdate.launch();
#endif

    // Run the GUI main message loop
    gui_run();

    // Uninstall support to drop files
    uninstall_drop_files();

    // Remove the root-menu from the menu-bar (because the rootmenu
    // module should destroy it).
    menubar->setMenu(NULL);

    // Delete all editors first because they used signals from other
    // widgets (e.g. color bar).
    jwidget_free(box_editors);

    // Destroy the top-window
    jwidget_free(top_window);
    top_window = NULL;
  }

  return 0;
}

// Finishes the ASE application.
App::~App()
{
  try {
    ASSERT(m_instance == this);

    // Remove ASE handlers
    PRINTF("ASE: Uninstalling\n");

    // Fire App Exit signal
    App::instance()->Exit();

    // Finalize modules, configuration and core
    Editor::editor_cursor_exit();
    boundary_exit();

    delete m_tabsDelegate;
    delete m_legacy;
    delete m_modules;
    delete m_loggerModule;
    delete m_configModule;

    // Destroy the loaded gui.xml file.
    delete GuiXml::instance();
  
    m_instance = NULL;
  }
  catch (...) {
    allegro_message("Error closing ASE.\n(uncaught exception)");

    // no re-throw
  }
}

LoggerModule* App::getLogger() const
{
  ASSERT(m_loggerModule != NULL);
  return m_loggerModule;
}

tools::ToolBox* App::getToolBox() const
{
  ASSERT(m_modules != NULL);
  return &m_modules->m_toolbox;
}

RecentFiles* App::getRecentFiles() const
{
  ASSERT(m_modules != NULL);
  return &m_modules->m_recent_files;
}

/**
 * Updates palette and redraw the screen.
 */
void app_refresh_screen(const Document* document)
{
  ASSERT(screen != NULL);

  if (document && document->getSprite())
    set_current_palette(document->getSprite()->getCurrentPalette(), false);
  else
    set_current_palette(NULL, false);

  // redraw the screen
  jmanager_refresh_screen();
}

/**
 * Regenerates the label for each tab in the @em tabsbar.
 */
void app_rebuild_documents_tabs()
{
  UIContext* context = UIContext::instance();
  const Documents& docs = context->getDocuments();

  // Insert all other sprites
  for (Documents::const_iterator
	 it = docs.begin(), end = docs.end(); it != end; ++it) {
    const Document* document = *it;
    app_update_document_tab(document);
  }
}

void app_update_document_tab(const Document* document)
{
  std::string str = get_filename(document->getFilename());

  // Add an asterisk if the document is modified.
  if (document->isModified())
    str += "*";

  tabsbar->setTabText(str.c_str(), const_cast<Document*>(document));
}

bool app_rebuild_recent_list()
{
  MenuItem* list_menuitem = get_recent_list_menuitem();
  MenuItem* menuitem;

  // Update the recent file list menu item
  if (list_menuitem) {
    if (list_menuitem->hasSubmenuOpened())
      return false;

    Command* cmd_open_file = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);

    Menu* submenu = list_menuitem->getSubmenu();
    if (submenu) {
      list_menuitem->setSubmenu(NULL);
      submenu->deferDelete();
    }

    // Build the menu of recent files
    submenu = new Menu();
    list_menuitem->setSubmenu(submenu);

    RecentFiles::const_iterator it = App::instance()->getRecentFiles()->begin();
    RecentFiles::const_iterator end = App::instance()->getRecentFiles()->end();

    if (it != end) {
      Params params;

      for (; it != end; ++it) {
	const char* filename = it->c_str();

	params.set("filename", filename);

	menuitem = new MenuItem2(get_filename(filename), cmd_open_file, &params);
	submenu->addChild(menuitem);
      }
    }
    else {
      menuitem = new MenuItem2("Nothing", NULL, NULL);
      menuitem->setEnabled(false);
      submenu->addChild(menuitem);
    }
  }

  return true;
}

int app_get_current_image_type()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Document* document = context->getActiveDocument();
  if (document != NULL)
    return document->getSprite()->getImgType();
  else if (screen != NULL && bitmap_color_depth(screen) == 8)
    return IMAGE_INDEXED;
  else
    return IMAGE_RGB;
}

Frame* app_get_top_window() { return top_window; }
MenuBar* app_get_menubar() { return menubar; }
StatusBar* app_get_statusbar() { return statusbar; }
ColorBar* app_get_colorbar() { return colorbar; }
Widget* app_get_toolbar() { return toolbar; }
Tabs* app_get_tabsbar() { return tabsbar; }

void app_default_statusbar_message()
{
  app_get_statusbar()
    ->setStatusText(250, "%s %s | %s", PACKAGE, VERSION, COPYRIGHT);
}

int app_get_color_to_clear_layer(Layer* layer)
{
  /* all transparent layers are cleared with the mask color */
  Color color = Color::fromMask();
  
  /* the `Background' is erased with the `Background Color' */
  if (layer != NULL && layer->is_background())
    color = colorbar->getBgColor();

  return color_utils::color_for_layer(color, layer);
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
      Menu* popup_menu = get_document_tab_popup_menu();
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
    app_get_statusbar()->setStatusText(250, "%s",
				       static_cast<const char*>(document->getFilename()));
  }
  else {
    app_get_statusbar()->clearText();
  }
}

