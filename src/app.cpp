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

#include <allegro.h>
/* #include <allegro/internal/aintern.h> */
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "app/color_utils.h"
#include "ase_exception.h"
#include "check_args.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "core/cfg.h"
#include "core/drop_files.h"
#include "core/file_system.h"
#include "core/modules.h"
#include "file/file.h"
#include "file/file_formats_manager.h"
#include "gui/jinete.h"
#include "gui/jintern.h"
#include "log.h"
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
#include "tools/toolbox.h"
#include "ui_context.h"
#include "util/boundary.h"
#include "util/render.h"
#include "widgets/color_bar.h"
#include "widgets/editor.h"
#include "widgets/menuitem.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

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
  ToolBox m_toolbox;
  RasterModule m_raster;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
};

class TabsBarHandler : public ITabsHandler
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
static Widget* menubar = NULL;	      /* the menu bar widget */
static StatusBar* statusbar = NULL;   /* the status bar widget */
static ColorBar* colorbar = NULL;     /* the color bar widget */
static Widget* toolbar = NULL;	      /* the tool bar widget */
static Tabs* tabsbar = NULL;	      // The tabs bar widget

static char *palette_filename = NULL;

static TabsBarHandler* tabsHandler = NULL;

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

    std::auto_ptr<Palette> pal(Palette::load(palette_filename));
    if (pal.get() == NULL)
      throw AseException("Error loading default palette from: %s",
			 static_cast<const char*>(palette_filename));

    set_default_palette(pal.get());
  }

  /* set system palette to the default one */
  set_current_palette(NULL, true);
}

int App::run()
{
  // Initialize GUI interface
  if (isGui()) {
    Widget* view;
    Editor* editor;

    PRINTF("GUI mode\n");

    // Setup the GUI screen
    jmouse_set_cursor(JI_CURSOR_NORMAL);
    jmanager_refresh_screen();

    // Load main window
    top_window = static_cast<Frame*>(load_widget("main_window.xml", "main_window"));

    box_menubar = jwidget_find_name(top_window, "menubar");
    box_editors = jwidget_find_name(top_window, "editor");
    box_colorbar = jwidget_find_name(top_window, "colorbar");
    box_toolbar = jwidget_find_name(top_window, "toolbar");
    box_statusbar = jwidget_find_name(top_window, "statusbar");
    box_tabsbar = jwidget_find_name(top_window, "tabsbar");

    menubar = jmenubar_new();
    statusbar = new StatusBar();
    colorbar = new ColorBar(box_colorbar->getAlign());
    toolbar = toolbar_new();
    tabsbar = new Tabs(tabsHandler = new TabsBarHandler());
    view = editor_view_new();
    editor = create_new_editor();

    // configure all widgets to expansives
    jwidget_expansive(menubar, true);
    jwidget_expansive(statusbar, true);
    jwidget_expansive(colorbar, true);
    jwidget_expansive(toolbar, true);
    jwidget_expansive(tabsbar, true);
    jwidget_expansive(view, true);

    /* prepare the first editor */
    jview_attach(view, editor);

    /* setup the menus */
    jmenubar_set_menu(menubar, get_root_menu());

    /* start text of status bar */
    app_default_statusbar_message();

    /* add the widgets in the boxes */
    if (box_menubar) jwidget_add_child(box_menubar, menubar);
    if (box_editors) jwidget_add_child(box_editors, view);
    if (box_colorbar) jwidget_add_child(box_colorbar, colorbar);
    if (box_toolbar) jwidget_add_child(box_toolbar, toolbar);
    if (box_statusbar) jwidget_add_child(box_statusbar, statusbar);
    if (box_tabsbar) jwidget_add_child(box_tabsbar, tabsbar);

    /* prepare the window */
    top_window->remap_window();

    /* rebuild menus */
    app_realloc_sprite_list();
    app_realloc_recent_list();

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
	  /* load the sprite */
	  Sprite* sprite = sprite_load(option->data().c_str());
	  if (!sprite) {
	    if (!isGui())
	      console.printf("Error loading file \"%s\"\n", option->data().c_str());
	  }
	  else {
	    // Mount and select the sprite
	    UIContext* context = UIContext::instance();
	    context->addSprite(sprite);
	    context->setCurrentSprite(sprite);

	    if (isGui()) {
	      // Show it
	      set_sprite_in_more_reliable_editor(context->getFirstSprite());

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

    gui_run();

    uninstall_drop_files();

    // Remove the root-menu from the menu-bar (because the rootmenu
    // module should destroy it).
    jmenubar_set_menu(menubar, NULL);

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

    delete tabsHandler;
    delete m_legacy;
    delete m_modules;
    delete m_loggerModule;
    delete m_configModule;
  
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

ToolBox* App::getToolBox() const
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
void app_refresh_screen(const Sprite* sprite)
{
  ASSERT(screen != NULL);

  if (sprite)
    set_current_palette(sprite->getCurrentPalette(), false);
  else
    set_current_palette(NULL, false);

  // redraw the screen
  jmanager_refresh_screen();
}

/**
 * Regenerates the label for each tab in the @em tabsbar.
 */
void app_realloc_sprite_list()
{
  UIContext* context = UIContext::instance();
  const SpriteList& list = context->getSpriteList();

  // Insert all other sprites
  for (SpriteList::const_iterator
	 it = list.begin(); it != list.end(); ++it) {
    Sprite* sprite = *it;
    tabsbar->setTabText(get_filename(sprite->getFilename()), sprite);
  }
}

/**
 * Updates the recent list menu.
 *
 * @warning This routine can't be used when a menu callback was
 * called, because, it destroy the menus, you should use
 * schedule_rebuild_recent_list() instead (src/modules/gui.c).
 */
bool app_realloc_recent_list()
{
  Widget* list_menuitem = get_recent_list_menuitem();
  Widget* menuitem;

  /* update the recent file list menu item */
  if (list_menuitem) {
    if (jmenuitem_has_submenu_opened(list_menuitem))
      return false;

    Command* cmd_open_file = CommandsModule::instance()->get_command_by_name(CommandId::OpenFile);

    Widget* submenu = jmenuitem_get_submenu(list_menuitem);
    if (submenu) {
      jmenuitem_set_submenu(list_menuitem, NULL);
      jwidget_free(submenu);
    }

    // Build the menu of recent files
    submenu = jmenu_new();
    jmenuitem_set_submenu(list_menuitem, submenu);

    RecentFiles::const_iterator it = App::instance()->getRecentFiles()->begin();
    RecentFiles::const_iterator end = App::instance()->getRecentFiles()->end();

    if (it != end) {
      Params params;

      for (; it != end; ++it) {
	const char* filename = it->c_str();

	params.set("filename", filename);

	menuitem = menuitem_new(get_filename(filename), cmd_open_file, &params);
	jwidget_add_child(submenu, menuitem);
      }
    }
    else {
      menuitem = menuitem_new("Nothing", NULL, NULL);
      menuitem->setEnabled(false);
      jwidget_add_child(submenu, menuitem);
    }
  }

  return true;
}

int app_get_current_image_type()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Sprite* sprite = context->getCurrentSprite();
  if (sprite != NULL)
    return sprite->getImgType();
  else if (screen != NULL && bitmap_color_depth(screen) == 8)
    return IMAGE_INDEXED;
  else
    return IMAGE_RGB;
}

Frame* app_get_top_window() { return top_window; }
Widget* app_get_menubar() { return menubar; }
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
// TabsBarHandler

void TabsBarHandler::clickTab(Tabs* tabs, void* data, int button)
{
  Sprite* sprite = (Sprite*)data;

  // put as current sprite
  set_sprite_in_more_reliable_editor(sprite);

  // middle-button: close the sprite
  if (data && (button & 4)) {
    Command* close_file_cmd =
      CommandsModule::instance()->get_command_by_name(CommandId::CloseFile);

    UIContext::instance()->executeCommand(close_file_cmd, NULL);
  }
}

void TabsBarHandler::mouseOverTab(Tabs* tabs, void* data)
{
  // Note: data can be NULL
  Sprite* sprite = (Sprite*)data;

  if (data) {
    app_get_statusbar()->setStatusText(250, "%s",
				       static_cast<const char*>(sprite->getFilename()));
  }
  else {
    app_get_statusbar()->clearText();
  }
}

