// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"

#include "app/app_options.h"
#include "app/check_update.h"
#include "app/color_utils.h"
#include "app/commands/cmd_save_file.h"
#include "app/commands/cmd_sprite_size.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/crash/data_recovery.h"
#include "app/document_exporter.h"
#include "app/document_location.h"
#include "app/document_undo.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "app/file/palette_file.h"
#include "app/file_system.h"
#include "app/filename_formatter.h"
#include "app/find_widget.h"
#include "app/gui_xml.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/log.h"
#include "app/modules.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "app/settings/settings.h"
#include "app/shell.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui_context.h"
#include "app/util/boundary.h"
#include "app/webserver.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/split_string.h"
#include "base/unique_ptr.h"
#include "doc/document_observer.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/render.h"
#include "scripting/engine.h"
#include "she/display.h"
#include "she/error.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <iostream>

namespace app {

using namespace ui;

class App::CoreModules {
public:
  ConfigModule m_configModule;
  Preferences m_preferences;
};

class App::Modules {
public:
  LoggerModule m_loggerModule;
  FileSystemModule m_file_system_module;
  tools::ToolBox m_toolbox;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
  app::crash::DataRecovery m_recovery;
  scripting::Engine m_scriptingEngine;

  Modules(bool console, bool verbose)
    : m_loggerModule(verbose)
    , m_recovery(&m_ui_context) {
  }
};

App* App::m_instance = NULL;

App::App()
  : m_coreModules(NULL)
  , m_modules(NULL)
  , m_legacy(NULL)
  , m_isGui(false)
  , m_isShell(false)
  , m_exporter(NULL)
{
  ASSERT(m_instance == NULL);
  m_instance = this;
}

void App::initialize(const AppOptions& options)
{
  if (options.startUI())
    m_guiSystem.reset(new ui::GuiSystem);

  // Initializes the application loading the modules, setting the
  // graphics mode, loading the configuration and resources, etc.
  m_coreModules = new CoreModules;
  m_modules = new Modules(!options.startUI(), options.verbose());
  m_isGui = options.startUI();
  m_isShell = options.startShell();
  m_legacy = new LegacyModules(isGui() ? REQUIRE_INTERFACE: 0);

  if (options.hasExporterParams())
    m_exporter.reset(new DocumentExporter);

  // Register well-known image file types.
  FileFormatsManager::instance()->registerAllFormats();

  // init editor cursor
  Editor::editor_cursor_init();

  if (isPortable())
    PRINTF("Running in portable mode\n");

  // Default palette.
  std::string palFile(!options.paletteFileName().empty() ?
    options.paletteFileName():
    std::string(get_config_string("GfxMode", "Palette", "")));

  if (palFile.empty()) {
    // Try to use a default pixel art palette.
    ResourceFinder rf;
    rf.includeDataDir("palettes/db32.gpl");
    if (rf.findFirst())
      palFile = rf.filename();
  }

  if (!palFile.empty()) {
    PRINTF("Loading custom palette file: %s\n", palFile.c_str());

    base::UniquePtr<Palette> pal(load_palette(palFile.c_str()));
    if (pal.get() != NULL) {
      set_default_palette(pal.get());
    }
    else {
      PRINTF("Error loading custom palette file\n");
    }
  }

  // Set system palette to the default one.
  set_current_palette(NULL, true);

  // Initialize GUI interface
  UIContext* ctx = UIContext::instance();
  if (isGui()) {
    PRINTF("GUI mode\n");

    // Setup the GUI cursor and redraw screen

    ui::set_use_native_cursors(
      preferences().experimental.useNativeCursor());

    ui::set_mouse_cursor(kArrowCursor);

    ui::Manager::getDefault()->invalidate();

    // Create the main window and show it.
    m_mainWindow.reset(new MainWindow);

    // Default status of the main window.
    app_rebuild_documents_tabs();
    app_default_statusbar_message();

    // Default window title bar.
    updateDisplayTitleBar();

    // Recover data
    if (!m_modules->m_recovery.sessions().empty())
      m_mainWindow->showDataRecovery(&m_modules->m_recovery);

    m_mainWindow->openWindow();

    // Redraw the whole screen.
    ui::Manager::getDefault()->invalidate();
  }

  // Procress options
  PRINTF("Processing options...\n");

  bool ignoreEmpty = false;
  bool trim = false;
  Params cropParams;

  // Open file specified in the command line
  if (!options.values().empty()) {
    Console console;
    bool splitLayers = false;
    bool splitLayersSaveAs = false;
    std::string importLayer;
    std::string importLayerSaveAs;
    std::string filenameFormat;

    for (const auto& value : options.values()) {
      const AppOptions::Option* opt = value.option();

      // Special options/commands
      if (opt) {
        // --data <file.json>
        if (opt == &options.data()) {
          if (m_exporter)
            m_exporter->setDataFilename(value.value());
        }
        // --sheet <file.png>
        else if (opt == &options.sheet()) {
          if (m_exporter)
            m_exporter->setTextureFilename(value.value());
        }
        // --sheet-width <width>
        else if (opt == &options.sheetWidth()) {
          if (m_exporter)
            m_exporter->setTextureWidth(strtol(value.value().c_str(), NULL, 0));
        }
        // --sheet-height <height>
        else if (opt == &options.sheetHeight()) {
          if (m_exporter)
            m_exporter->setTextureHeight(strtol(value.value().c_str(), NULL, 0));
        }
        // --sheet-pack
        else if (opt == &options.sheetPack()) {
          if (m_exporter)
            m_exporter->setTexturePack(true);
        }
        // --split-layers
        else if (opt == &options.splitLayers()) {
          splitLayers = true;
          splitLayersSaveAs = true;
        }
        // --import-layer <layer-name>
        else if (opt == &options.importLayer()) {
          importLayer = value.value();
          importLayerSaveAs = value.value();
        }
        // --ignore-empty
        else if (opt == &options.ignoreEmpty()) {
          ignoreEmpty = true;
        }
        // --border-padding
        else if (opt == &options.borderPadding()) {
          if (m_exporter)
            m_exporter->setBorderPadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --shape-padding
        else if (opt == &options.shapePadding()) {
          if (m_exporter)
            m_exporter->setShapePadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --inner-padding
        else if (opt == &options.innerPadding()) {
          if (m_exporter)
            m_exporter->setInnerPadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --trim
        else if (opt == &options.trim()) {
          trim = true;
        }
        // --crop
        else if (opt == &options.crop()) {
          std::vector<std::string> parts;
          base::split_string(value.value(), parts, ",");
          if (parts.size() == 4) {
            cropParams.set("x", parts[0].c_str());
            cropParams.set("y", parts[1].c_str());
            cropParams.set("width", parts[2].c_str());
            cropParams.set("height", parts[3].c_str());
          }
        }
        // --filename-format
        else if (opt == &options.filenameFormat()) {
          filenameFormat = value.value();
        }
        // --save-as <filename>
        else if (opt == &options.saveAs()) {
          Document* doc = NULL;
          if (!ctx->documents().empty())
            doc = dynamic_cast<Document*>(ctx->documents().lastAdded());

          if (!doc) {
            console.printf("A document is needed before --save-as argument\n");
          }
          else {
            ctx->setActiveDocument(doc);

            std::string format = filenameFormat;

            Command* saveAsCommand = CommandsModule::instance()->getCommandByName(CommandId::SaveFileCopyAs);
            Command* trimCommand = CommandsModule::instance()->getCommandByName(CommandId::AutocropSprite);
            Command* cropCommand = CommandsModule::instance()->getCommandByName(CommandId::CropSprite);
            Command* undoCommand = CommandsModule::instance()->getCommandByName(CommandId::Undo);

            if (splitLayersSaveAs) {
              std::vector<Layer*> layers;
              doc->sprite()->getLayersList(layers);

              std::string fn, fmt;
              if (format.empty()) {
                if (doc->sprite()->totalFrames() > frame_t(1))
                  format = "{path}/{title} ({layer}) {frame}.{extension}";
                else
                  format = "{path}/{title} ({layer}).{extension}";
              }

              // For each layer, hide other ones and save the sprite.
              for (Layer* show : layers) {
                for (Layer* hide : layers)
                  hide->setVisible(hide == show);

                fn = filename_formatter(format,
                  value.value(), show->name());
                fmt = filename_formatter(format,
                  value.value(), show->name(), -1, false);

                if (!cropParams.empty())
                  ctx->executeCommand(cropCommand, cropParams);

                // TODO --trim command with --save-as doesn't make too
                // much sense as we lost the trim rectangle
                // information (e.g. we don't have sheet .json) Also,
                // we should trim each frame individually (a process
                // that can be done only in fop_operate()).
                if (trim)
                  ctx->executeCommand(trimCommand);

                Params params;
                params.set("filename", fn.c_str());
                params.set("filename-format", fmt.c_str());
                ctx->executeCommand(saveAsCommand, params);

                if (trim) {     // Undo trim command
                  ctx->executeCommand(undoCommand);

                  // Just in case allow non-linear history is enabled
                  // we clear redo information
                  doc->undoHistory()->clearRedo();
                }
              }
            }
            else {
              std::vector<Layer*> layers;
              doc->sprite()->getLayersList(layers);

              // Show only one layer
              if (!importLayerSaveAs.empty()) {
                for (Layer* layer : layers)
                  layer->setVisible(layer->name() == importLayerSaveAs);
              }

              if (!cropParams.empty())
                ctx->executeCommand(cropCommand, cropParams);

              if (trim)
                ctx->executeCommand(trimCommand);

              Params params;
              params.set("filename", value.value().c_str());
              params.set("filename-format", format.c_str());
              ctx->executeCommand(saveAsCommand, params);

              if (trim) {       // Undo trim command
                ctx->executeCommand(undoCommand);

                // Just in case allow non-linear history is enabled
                // we clear redo information
                doc->undoHistory()->clearRedo();
              }
            }
          }
        }
        // --scale <factor>
        else if (opt == &options.scale()) {
          Command* command = CommandsModule::instance()->getCommandByName(CommandId::SpriteSize);
          double scale = strtod(value.value().c_str(), NULL);
          static_cast<SpriteSizeCommand*>(command)->setScale(scale, scale);

          // Scale all sprites
          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(doc);
            ctx->executeCommand(command);
          }
        }
      }
      // File names aren't associated to any option
      else {
        const std::string& filename = value.value();

        // Load the sprite
        Document* doc = load_document(ctx, filename.c_str());
        if (!doc) {
          if (!isGui())
            console.printf("Error loading file \"%s\"\n", filename.c_str());
        }
        else {
          // Add the given file in the argument as a "recent file" only
          // if we are running in GUI mode. If the program is executed
          // in batch mode this is not desirable.
          if (isGui())
            getRecentFiles()->addRecentFile(filename.c_str());

          if (m_exporter != NULL) {
            if (!importLayer.empty()) {
              std::vector<Layer*> layers;
              doc->sprite()->getLayersList(layers);

              Layer* foundLayer = NULL;
              for (Layer* layer : layers) {
                if (layer->name() == importLayer) {
                  foundLayer = layer;
                  break;
                }
              }
              if (foundLayer)
                m_exporter->addDocument(doc, foundLayer);
            }
            else if (splitLayers) {
              std::vector<Layer*> layers;
              doc->sprite()->getLayersList(layers);
              for (auto layer : layers)
                m_exporter->addDocument(doc, layer);
            }
            else
              m_exporter->addDocument(doc);
          }
        }

        if (!importLayer.empty())
          importLayer.clear();

        if (splitLayers)
          splitLayers = false;
      }
    }

    if (m_exporter && !filenameFormat.empty())
      m_exporter->setFilenameFormat(filenameFormat);
  }

  // Export
  if (m_exporter) {
    PRINTF("Exporting sheet...\n");

    if (ignoreEmpty)
      m_exporter->setIgnoreEmptyCels(true);

    if (trim)
      m_exporter->setTrimCels(true);

    base::UniquePtr<Document> spriteSheet(m_exporter->exportSheet());
    m_exporter.reset(NULL);

    PRINTF("Export sprite sheet: Done\n");
  }
}

void App::run()
{
  // Run the GUI
  if (isGui()) {
#ifdef ENABLE_UPDATER
    // Launch the thread to check for updates.
    app::CheckUpdateThreadLauncher checkUpdate(
      m_mainWindow->getCheckUpdateDelegate());
    checkUpdate.launch();
#endif

#ifdef ENABLE_WEBSERVER
    // Launch the webserver.
    app::WebServer webServer;
    webServer.start();
#endif

    app::SendCrash sendCrash;
    sendCrash.search();

    // Run the GUI main message loop
    gui_run();
  }

  // Start shell to execute scripts.
  if (m_isShell) {
    if (m_modules->m_scriptingEngine.supportEval()) {
      Shell shell;
      shell.run(m_modules->m_scriptingEngine);
    }
    else {
      std::cerr << "Your version of " PACKAGE " wasn't compiled with shell support.\n";
    }
  }

  // Destroy all documents in the UIContext.
  const doc::Documents& docs = m_modules->m_ui_context.documents();
  while (!docs.empty()) {
    doc::Document* doc = docs.back();

    // First we close the document. In this way we receive recent
    // notifications related to the document as an app::Document. If
    // we delete the document directly, we destroy the app::Document
    // too early, and then doc::~Document() call
    // DocumentsObserver::onRemoveDocument(). In this way, observers
    // could think that they have a fully created app::Document when
    // in reality it's a doc::Document (in the middle of a
    // destruction process).
    //
    // TODO: This problem is because we're extending doc::Document,
    // in the future, we should remove app::Document.
    doc->close();
    delete doc;
  }

  if (isGui()) {
    // Destroy the window.
    m_mainWindow.reset(NULL);
  }
}

// Finishes the Aseprite application.
App::~App()
{
  try {
    ASSERT(m_instance == this);

    // Remove Aseprite handlers
    PRINTF("ASE: Uninstalling\n");

    // Delete file formats.
    FileFormatsManager::destroyInstance();

    // Fire App Exit signal.
    App::instance()->Exit();

    // Finalize modules, configuration and core.
    Editor::editor_cursor_exit();
    boundary_exit();

    delete m_legacy;
    delete m_modules;
    delete m_coreModules;

    // Destroy the loaded gui.xml data.
    delete KeyboardShortcuts::instance();
    delete GuiXml::instance();

    m_instance = NULL;
  }
  catch (const std::exception& e) {
    she::error_message(e.what());

    // no re-throw
  }
  catch (...) {
    she::error_message("Error closing ASE.\n(uncaught exception)");

    // no re-throw
  }
}

bool App::isPortable()
{
  static bool* is_portable = NULL;
  if (!is_portable) {
    is_portable =
      new bool(
        base::is_file(base::join_path(
            base::get_file_path(base::get_app_path()),
            "aseprite.ini")));
  }
  return *is_portable;
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

Preferences& App::preferences() const
{
  return m_coreModules->m_preferences;
}

void App::showNotification(INotificationDelegate* del)
{
  m_mainWindow->showNotification(del);
}

void App::updateDisplayTitleBar()
{
  std::string defaultTitle = PACKAGE " v" VERSION;
  std::string title;

  DocumentView* docView = UIContext::instance()->activeView();
  if (docView) {
    // Prepend the document's filename.
    title += docView->getDocument()->name();
    title += " - ";
  }

  title += defaultTitle;
  she::instance()->defaultDisplay()->setTitleBar(title);
}

// Updates palette and redraw the screen.
void app_refresh_screen()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  DocumentLocation location = context->activeLocation();

  if (Palette* pal = location.palette())
    set_current_palette(pal, false);
  else
    set_current_palette(NULL, false);

  // Invalidate the whole screen.
  ui::Manager::getDefault()->invalidate();
}

void app_rebuild_documents_tabs()
{
  if (App::instance()->isGui())
    App::instance()->getMainWindow()->getTabsBar()->updateTabs();
}

PixelFormat app_get_current_pixel_format()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Document* document = context->activeDocument();
  if (document != NULL)
    return document->sprite()->pixelFormat();
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
  ASSERT(layer != NULL);

  app::Color color;

  // The `Background' is erased with the `Background Color'
  if (layer->isBackground()) {
    if (ColorBar::instance())
      color = ColorBar::instance()->getBgColor();
    else
      color = app::Color::fromRgb(0, 0, 0); // TODO get background color color from doc::Settings
  }
  else // All transparent layers are cleared with the mask color
    color = app::Color::fromMask();

  return color_utils::color_for_layer(color, layer);
}

} // namespace app
