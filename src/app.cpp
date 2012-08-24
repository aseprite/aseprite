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

#include "app.h"

#include "app/check_update.h"
#include "app/color_utils.h"
#include "app/data_recovery.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "base/exception.h"
#include "base/unique_ptr.h"
#include "check_args.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "document_observer.h"
#include "drop_files.h"
#include "file/file.h"
#include "file/file_formats_manager.h"
#include "file_system.h"
#include "gui_xml.h"
#include "ini_file.h"
#include "log.h"
#include "modules.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "recent_files.h"
#include "tools/tool_box.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "ui_context.h"
#include "util/boundary.h"
#include "util/render.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/editor/editor_view.h"
#include "widgets/main_window.h"
#include "widgets/menuitem2.h"
#include "widgets/status_bar.h"
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

using namespace ui;

class App::Modules
{
public:
  FileSystemModule m_file_system_module;
  tools::ToolBox m_toolbox;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
  app::DataRecovery m_recovery;

  Modules()
    : m_recovery(&m_ui_context) {
  }
};

App* App::m_instance = NULL;

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
  // Initialize GUI interface
  if (isGui()) {
    PRINTF("GUI mode\n");

    // Setup the GUI screen
    jmouse_set_cursor(kArrowCursor);
    ui::Manager::getDefault()->invalidate();

    // Create the main window and show it.
    m_mainWindow.reset(new MainWindow);
    m_mainWindow->createFirstEditor();
    m_mainWindow->openWindow();

    // Redraw the whole screen.
    ui::Manager::getDefault()->invalidate();
  }

  /* set background mode for non-GUI modes */
/*   if (!(ase_mode & MODE_GUI)) */
/*     set_display_switch_mode(SWITCH_BACKAMNESIA); */
    set_display_switch_mode(SWITCH_BACKGROUND);

  // Procress options
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
    app::CheckUpdateThreadLauncher checkUpdate;
    checkUpdate.launch();
#endif

    // Run the GUI main message loop
    gui_run();

    // Uninstall support to drop files
    uninstall_drop_files();

    // Destroy the window.
    m_mainWindow.reset(NULL);
  }

  return 0;
}

// Finishes the ASEPRITE application.
App::~App()
{
  try {
    ASSERT(m_instance == this);

    // Remove ASEPRITE handlers
    PRINTF("ASE: Uninstalling\n");

    // Fire App Exit signal.
    App::instance()->Exit();

    // Finalize modules, configuration and core.
    Editor::editor_cursor_exit();
    boundary_exit();

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

  // Invalidate the whole screen.
  ui::Manager::getDefault()->invalidate();
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

  App::instance()->getMainWindow()->getTabsBar()
    ->setTabText(str.c_str(), const_cast<Document*>(document));
}

PixelFormat app_get_current_pixel_format()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Document* document = context->getActiveDocument();
  if (document != NULL)
    return document->getSprite()->getPixelFormat();
  else if (screen != NULL && bitmap_color_depth(screen) == 8)
    return IMAGE_INDEXED;
  else
    return IMAGE_RGB;
}

void app_default_statusbar_message()
{
  StatusBar::instance()
    ->setStatusText(250, "%s %s | %s", PACKAGE, VERSION, COPYRIGHT);
}

int app_get_color_to_clear_layer(Layer* layer)
{
  /* all transparent layers are cleared with the mask color */
  Color color = Color::fromMask();

  /* the `Background' is erased with the `Background Color' */
  if (layer != NULL && layer->is_background())
    color = ColorBar::instance()->getBgColor();

  return color_utils::color_for_layer(color, layer);
}
