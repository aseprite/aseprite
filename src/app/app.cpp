/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"

#include "app/app_options.h"
#include "app/check_update.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/data_recovery.h"
#include "app/document_location.h"
#include "app/document_observer.h"
#include "app/drop_files.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "app/file_system.h"
#include "app/find_widget.h"
#include "app/gui_xml.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/log.h"
#include "app/modules.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/recent_files.h"
#include "app/shell.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/tabs.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "app/util/boundary.h"
#include "app/util/render.h"
#include "app/webserver.h"
#include "base/exception.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "scripting/engine.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <allegro.h>
#include <iostream>
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef ALLEGRO_WINDOWS
  #include <winalleg.h>
#endif

namespace app {

using namespace ui;

class App::Modules {
public:
  ConfigModule m_configModule;
  LoggerModule m_loggerModule;
  FileSystemModule m_file_system_module;
  tools::ToolBox m_toolbox;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
  app::DataRecovery m_recovery;
  scripting::Engine m_scriptingEngine;

  Modules(bool console, bool verbose)
    : m_loggerModule(verbose)
    , m_recovery(&m_ui_context) {
  }
};

App* App::m_instance = NULL;

// Initializes the application loading the modules, setting the
// graphics mode, loading the configuration and resources, etc.
App::App(int argc, const char* argv[])
  : m_modules(NULL)
  , m_legacy(NULL)
  , m_isGui(false)
  , m_isShell(false)
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  AppOptions options(argc, argv);

  m_modules = new Modules(!options.startUI(), options.verbose());
  m_isGui = options.startUI();
  m_isShell = options.startShell();
  m_legacy = new LegacyModules(isGui() ? REQUIRE_INTERFACE: 0);
  m_files = options.files();

  // Register well-known image file types.
  FileFormatsManager::instance().registerAllFormats();

  // init editor cursor
  Editor::editor_cursor_init();

  // Load RenderEngine configuration
  RenderEngine::loadConfig();

  // Default palette.
  base::string palFile(!options.paletteFileName().empty() ?
                       options.paletteFileName():
                       base::string(get_config_string("GfxMode", "Palette", "")));

  if (!palFile.empty()) {
    PRINTF("Loading custom palette file: %s\n", palFile.c_str());

    base::UniquePtr<Palette> pal(Palette::load(palFile.c_str()));
    if (pal.get() == NULL)
      throw base::Exception("Error loading default palette from: %s", palFile);

    set_default_palette(pal.get());
  }

  // Set system palette to the default one.
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

    // Default status of the main window.
    app_rebuild_documents_tabs();
    app_default_statusbar_message();

    m_mainWindow->openWindow();

    // Redraw the whole screen.
    ui::Manager::getDefault()->invalidate();
  }

  // Set background mode for non-GUI modes
  set_display_switch_mode(SWITCH_BACKGROUND);

  // Procress options
  PRINTF("Processing options...\n");

  {
    Console console;
    for (FileList::iterator
           it  = m_files.begin(),
           end = m_files.end();
         it != end; ++it) {
      // Load the sprite
      Document* document = load_document(it->c_str());
      if (!document) {
        if (!isGui())
          console.printf("Error loading file \"%s\"\n", it->c_str());
      }
      else {
        // Mount and select the sprite
        UIContext* context = UIContext::instance();
        context->addDocument(document);

        if (isGui()) {
          // Recent file
          getRecentFiles()->addRecentFile(it->c_str());
        }
      }
    }
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

#ifdef ENABLE_WEBSERVER
    // Launch the webserver.
    app::WebServer webServer;
    webServer.start();
#endif

    // Run the GUI main message loop
    gui_run();

    // Uninstall support to drop files
    uninstall_drop_files();

    // Destroy all documents in the UIContext.
    const Documents& docs = m_modules->m_ui_context.getDocuments();
    while (!docs.empty())
      m_modules->m_ui_context.removeDocument(docs.back());

    // Destroy the window.
    m_mainWindow.reset(NULL);
  }
  // Start shell to execute scripts.
  else if (m_isShell) {
    m_systemConsole.prepareShell();

    if (m_modules->m_scriptingEngine.supportEval()) {
      Shell shell;
      shell.run(m_modules->m_scriptingEngine);
    }
    else {
      std::cerr << "Your version of " PACKAGE " wasn't compiled with shell support.\n";
    }
  }

  return 0;
}

// Finishes the Aseprite application.
App::~App()
{
  try {
    ASSERT(m_instance == this);

    // Remove Aseprite handlers
    PRINTF("ASE: Uninstalling\n");

    // Fire App Exit signal.
    App::instance()->Exit();

    // Finalize modules, configuration and core.
    Editor::editor_cursor_exit();
    boundary_exit();

    delete m_legacy;
    delete m_modules;

    // Destroy the loaded gui.xml file.
    delete GuiXml::instance();

    m_instance = NULL;
  }
  catch (...) {
    allegro_message("Error closing ASE.\n(uncaught exception)");

    // no re-throw
  }
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

// Updates palette and redraw the screen.
void app_refresh_screen()
{
  ASSERT(screen != NULL);

  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  DocumentLocation location = context->getActiveLocation();

  if (Palette* pal = location.palette())
    set_current_palette(pal, false);
  else
    set_current_palette(NULL, false);

  // Invalidate the whole screen.
  ui::Manager::getDefault()->invalidate();
}

void app_rebuild_documents_tabs()
{
  App::instance()->getMainWindow()->getTabsBar()->updateTabsText();
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
  app::Color color = app::Color::fromMask();

  /* the `Background' is erased with the `Background Color' */
  if (layer != NULL && layer->isBackground())
    color = ColorBar::instance()->getBgColor();

  return color_utils::color_for_layer(color, layer);
}

} // namespace app
