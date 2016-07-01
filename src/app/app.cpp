// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "app/document_undo.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "app/file_system.h"
#include "app/filename_formatter.h"
#include "app/gui_xml.h"
#include "app/ini_file.h"
#include "app/log.h"
#include "app/modules.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/script/app_scripting.h"
#include "app/send_crash.h"
#include "app/shell.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/input_chain.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/webserver.h"
#include "base/convert_to.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/split_string.h"
#include "base/unique_ptr.h"
#include "doc/document_observer.h"
#include "doc/frame_tag.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layers_range.h"
#include "doc/palette.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "render/render.h"
#include "script/engine_delegate.h"
#include "she/display.h"
#include "she/error.h"
#include "she/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <iostream>

#ifdef ENABLE_STEAM
  #include "steam/steam.h"
#endif

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
  tools::ActiveToolManager m_activeToolManager;
  CommandsModule m_commands_modules;
  UIContext m_ui_context;
  RecentFiles m_recent_files;
  InputChain m_inputChain;
  clipboard::ClipboardManager m_clipboardManager;
  // This is a raw pointer because we want to delete this explicitly.
  app::crash::DataRecovery* m_recovery;

  Modules(bool createLogInDesktop)
    : m_loggerModule(createLogInDesktop)
    , m_activeToolManager(&m_toolbox)
    , m_recovery(nullptr) {
  }

  app::crash::DataRecovery* recovery() {
    return m_recovery;
  }

  bool hasRecoverySessions() const {
    return m_recovery && !m_recovery->sessions().empty();
  }

  void createDataRecovery() {
#ifdef ENABLE_DATA_RECOVERY
    m_recovery = new app::crash::DataRecovery(&m_ui_context);
#endif
  }

  void deleteDataRecovery() {
#ifdef ENABLE_DATA_RECOVERY
    delete m_recovery;
#endif
  }

};

class StdoutEngineDelegate : public script::EngineDelegate {
public:
  void onConsolePrint(const char* text) override {
    printf("%s\n", text);
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
  m_isGui = options.startUI();
  m_isShell = options.startShell();
  if (m_isGui)
    m_uiSystem.reset(new ui::UISystem);

  m_coreModules = new CoreModules;

  bool createLogInDesktop = false;
  switch (options.verboseLevel()) {
    case AppOptions::kNoVerbose:
      base::set_log_level(ERROR);
      break;
    case AppOptions::kVerbose:
      base::set_log_level(INFO);
      break;
    case AppOptions::kHighlyVerbose:
      base::set_log_level(VERBOSE);
      createLogInDesktop = true;
      break;
  }

  m_modules = new Modules(createLogInDesktop);
  m_legacy = new LegacyModules(isGui() ? REQUIRE_INTERFACE: 0);
  m_brushes.reset(new AppBrushes);

  if (options.hasExporterParams())
    m_exporter.reset(new DocumentExporter);

  // Data recovery is enabled only in GUI mode
  if (isGui() && preferences().general.dataRecovery())
    m_modules->createDataRecovery();

  // Register well-known image file types.
  FileFormatsManager::instance()->registerAllFormats();

  if (isPortable())
    LOG("Running in portable mode\n");

  // Load or create the default palette, or migrate the default
  // palette from an old format palette to the new one, etc.
  load_default_palette(options.paletteFileName());

  // Initialize GUI interface
  UIContext* ctx = UIContext::instance();
  if (isGui()) {
    LOG("GUI mode\n");

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

    // Recover data
    if (m_modules->hasRecoverySessions())
      m_mainWindow->showDataRecovery(m_modules->recovery());

    m_mainWindow->openWindow();

    // Redraw the whole screen.
    ui::Manager::getDefault()->invalidate();
  }

  // Procress options
  LOG("Processing options...\n");

  bool ignoreEmpty = false;
  bool trim = false;
  Params cropParams;
  SpriteSheetType sheetType = SpriteSheetType::None;

  // Open file specified in the command line
  if (!options.values().empty()) {
    Console console;
    bool splitLayers = false;
    bool splitLayersSaveAs = false;
    bool allLayers = false;
    bool listLayers = false;
    bool listTags = false;
    std::string importLayer;
    std::string importLayerSaveAs;
    std::string filenameFormat;
    std::string frameTagName;
    std::string frameRange;

    for (const auto& value : options.values()) {
      const AppOptions::Option* opt = value.option();

      // Special options/commands
      if (opt) {
        // --data <file.json>
        if (opt == &options.data()) {
          if (m_exporter)
            m_exporter->setDataFilename(value.value());
        }
        // --format <format>
        else if (opt == &options.format()) {
          if (m_exporter) {
            DocumentExporter::DataFormat format = DocumentExporter::DefaultDataFormat;

            if (value.value() == "json-hash")
              format = DocumentExporter::JsonHashDataFormat;
            else if (value.value() == "json-array")
              format = DocumentExporter::JsonArrayDataFormat;

            m_exporter->setDataFormat(format);
          }
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
        else if (opt == &options.sheetType()) {
          if (value.value() == "horizontal")
            sheetType = SpriteSheetType::Horizontal;
          else if (value.value() == "vertical")
            sheetType = SpriteSheetType::Vertical;
          else if (value.value() == "rows")
            sheetType = SpriteSheetType::Rows;
          else if (value.value() == "columns")
            sheetType = SpriteSheetType::Columns;
          else if (value.value() == "packed")
            sheetType = SpriteSheetType::Packed;
        }
        // --sheet-pack
        else if (opt == &options.sheetPack()) {
          sheetType = SpriteSheetType::Packed;
        }
        // --split-layers
        else if (opt == &options.splitLayers()) {
          splitLayers = true;
          splitLayersSaveAs = true;
        }
        // --layer <layer-name>
        else if (opt == &options.layer()) {
          importLayer = value.value();
          importLayerSaveAs = value.value();
        }
        // --all-layers
        else if (opt == &options.allLayers()) {
          allLayers = true;
        }
        // --frame-tag <tag-name>
        else if (opt == &options.frameTag()) {
          frameTagName = value.value();
        }
        // --frame-range from,to
        else if (opt == &options.frameRange()) {
          frameRange = value.value();
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
        // --crop x,y,width,height
        else if (opt == &options.crop()) {
          std::vector<std::string> parts;
          base::split_string(value.value(), parts, ",");
          if (parts.size() < 4)
            throw std::runtime_error("--crop needs four parameters separated by comma (,)\n"
                                     "Usage: --crop x,y,width,height\n"
                                     "E.g. --crop 0,0,32,32");

          cropParams.set("x", parts[0].c_str());
          cropParams.set("y", parts[1].c_str());
          cropParams.set("width", parts[2].c_str());
          cropParams.set("height", parts[3].c_str());
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

            // --save-as with --split-layers
            if (splitLayersSaveAs) {
              std::string fn, fmt;
              if (format.empty()) {
                if (doc->sprite()->totalFrames() > frame_t(1))
                  format = "{path}/{title} ({layer}) {frame}.{extension}";
                else
                  format = "{path}/{title} ({layer}).{extension}";
              }

              // Store in "visibility" the original "visible" state of every layer.
              std::vector<bool> visibility(doc->sprite()->countLayers());
              int i = 0;
              for (Layer* layer : doc->sprite()->layers())
                visibility[i++] = layer->isVisible();

              // For each layer, hide other ones and save the sprite.
              i = 0;
              for (Layer* show : doc->sprite()->layers()) {
                // If the user doesn't want all layers and this one is hidden.
                if (!visibility[i++])
                  continue;     // Just ignore this layer.

                // Make this layer ("show") the only one visible.
                for (Layer* hide : doc->sprite()->layers())
                  hide->setVisible(hide == show);

                FilenameInfo fnInfo;
                fnInfo
                  .filename(value.value())
                  .layerName(show->name());

                fn = filename_formatter(format, fnInfo);
                fmt = filename_formatter(format, fnInfo, false);

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

              // Restore layer visibility
              i = 0;
              for (Layer* layer : doc->sprite()->layers())
                layer->setVisible(visibility[i++]);
            }
            else {
              // Show only one layer
              if (!importLayerSaveAs.empty()) {
                for (Layer* layer : doc->sprite()->layers())
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
            ctx->setActiveDocument(static_cast<app::Document*>(doc));
            ctx->executeCommand(command);
          }
        }
        // --shrink-to <width,height>
        else if (opt == &options.shrinkTo()) {
          std::vector<std::string> dimensions;
          base::split_string(value.value(), dimensions, ",");
          if (dimensions.size() < 2)
            throw std::runtime_error("--shrink-to needs two parameters separated by comma (,)\n"
                                     "Usage: --shrink-to width,height\n"
                                     "E.g. --shrink-to 128,64");

          double maxWidth = base::convert_to<double>(dimensions[0]);
          double maxHeight = base::convert_to<double>(dimensions[1]);
          double scaleWidth, scaleHeight, scale;

          // Shrink all sprites if needed
          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(static_cast<app::Document*>(doc));
            scaleWidth = (doc->width() > maxWidth ? maxWidth / doc->width() : 1.0);
            scaleHeight = (doc->height() > maxHeight ? maxHeight / doc->height() : 1.0);
            if (scaleWidth < 1.0 || scaleHeight < 1.0) {
              scale = MIN(scaleWidth, scaleHeight);
              Command* command = CommandsModule::instance()->getCommandByName(CommandId::SpriteSize);
              static_cast<SpriteSizeCommand*>(command)->setScale(scale, scale);
              ctx->executeCommand(command);
            }
          }
        }
        // --script <filename>
        else if (opt == &options.script()) {
          std::string script = value.value();

          StdoutEngineDelegate delegate;
          AppScripting engine(&delegate);
          engine.evalFile(script);
        }
        // --list-layers
        else if (opt == &options.listLayers()) {
          listLayers = true;
          if (m_exporter)
            m_exporter->setListLayers(true);
        }
        // --list-tags
        else if (opt == &options.listTags()) {
          listTags = true;
          if (m_exporter)
            m_exporter->setListFrameTags(true);
        }
      }
      // File names aren't associated to any option
      else {
        const std::string& filename = base::normalize_path(value.value());

        app::Document* oldDoc = ctx->activeDocument();

        Command* openCommand = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
        Params params;
        params.set("filename", filename.c_str());
        ctx->executeCommand(openCommand, params);

        app::Document* doc = ctx->activeDocument();

        // If the active document is equal to the previous one, it
        // means that we couldn't open this specific document.
        if (doc == oldDoc)
          doc = nullptr;

        // List layers and/or tags
        if (doc) {
          // Show all layers
          if (allLayers) {
            for (Layer* layer : doc->sprite()->layers())
              layer->setVisible(true);
          }

          if (listLayers) {
            listLayers = false;
            for (Layer* layer : doc->sprite()->layers()) {
              if (layer->isVisible())
                std::cout << layer->name() << "\n";
            }
          }

          if (listTags) {
            listTags = false;
            for (FrameTag* tag : doc->sprite()->frameTags())
              std::cout << tag->name() << "\n";
          }
          if (m_exporter) {
            FrameTag* frameTag = nullptr;
            if (!frameTagName.empty()) {
              frameTag = doc->sprite()->frameTags().getByName(frameTagName);
            }
            else if (!frameRange.empty()) {
                std::vector<std::string> splitRange;
                base::split_string(frameRange, splitRange, ",");
                if (splitRange.size() < 2)
                  throw std::runtime_error("--frame-range needs two parameters separated by comma (,)\n"
                                           "Usage: --frame-range from,to\n"
                                           "E.g. --frame-range 0,99");

                frameTag = new FrameTag(base::convert_to<frame_t>(splitRange[0]),
                                        base::convert_to<frame_t>(splitRange[1]));
            }

            if (!importLayer.empty()) {
              Layer* foundLayer = NULL;
              for (Layer* layer : doc->sprite()->layers()) {
                if (layer->name() == importLayer) {
                  foundLayer = layer;
                  break;
                }
              }
              if (foundLayer)
                m_exporter->addDocument(doc, foundLayer, frameTag);
            }
            else if (splitLayers) {
              for (auto layer : doc->sprite()->layers()) {
                if (layer->isVisible())
                  m_exporter->addDocument(doc, layer, frameTag);
              }
            }
            else {
              m_exporter->addDocument(doc, nullptr, frameTag);
            }
          }
        }

        if (!importLayer.empty())
          importLayer.clear();

        if (splitLayers)
          splitLayers = false;
        if (listLayers)
          listLayers = false;
        if (listTags)
          listTags = false;
      }
    }

    if (m_exporter && !filenameFormat.empty())
      m_exporter->setFilenameFormat(filenameFormat);
  }

  // Export
  if (m_exporter) {
    LOG("Exporting sheet...\n");

    if (sheetType != SpriteSheetType::None)
      m_exporter->setSpriteSheetType(sheetType);

    if (ignoreEmpty)
      m_exporter->setIgnoreEmptyCels(true);

    if (trim)
      m_exporter->setTrimCels(true);

    base::UniquePtr<Document> spriteSheet(m_exporter->exportSheet());
    m_exporter.reset(NULL);

    LOG("Export sprite sheet: Done\n");
  }

  she::instance()->finishLaunching();
}

void App::run()
{
  // Run the GUI
  if (isGui()) {
    // Initialize Steam API
#ifdef ENABLE_STEAM
    steam::SteamAPI steam;
    if (steam.initialized())
      she::instance()->activateApp();
#endif

#if _DEBUG
    // On OS X, when we compile Aseprite on Debug mode, we're using it
    // outside an app bundle, so we must active the app explicitly.
    she::instance()->activateApp();
#endif

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
    ui::Manager::getDefault()->run();
  }

  // Start shell to execute scripts.
  if (m_isShell) {
    StdoutEngineDelegate delegate;
    AppScripting engine(&delegate);
    engine.printLastResult();
    Shell shell;
    shell.run(engine);
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

  // Delete backups (this is a normal shutdown, we are not handling
  // exceptions, and we are not in a destructor).
  m_modules->deleteDataRecovery();
}

// Finishes the Aseprite application.
App::~App()
{
  try {
    ASSERT(m_instance == this);

    // Remove Aseprite handlers
    LOG("ASE: Uninstalling\n");

    // Delete file formats.
    FileFormatsManager::destroyInstance();

    // Fire App Exit signal.
    App::instance()->Exit();

    // Finalize modules, configuration and core.
    Editor::destroyEditorSharedInternals();

    // Save brushes
    m_brushes.reset(nullptr);

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

tools::ToolBox* App::toolBox() const
{
  ASSERT(m_modules != NULL);
  return &m_modules->m_toolbox;
}

tools::Tool* App::activeTool() const
{
  return m_modules->m_activeToolManager.activeTool();
}

tools::ActiveToolManager* App::activeToolManager() const
{
  return &m_modules->m_activeToolManager;
}

RecentFiles* App::recentFiles() const
{
  ASSERT(m_modules != NULL);
  return &m_modules->m_recent_files;
}

Workspace* App::workspace() const
{
  if (m_mainWindow)
    return m_mainWindow->getWorkspace();
  else
    return nullptr;
}

ContextBar* App::contextBar() const
{
  if (m_mainWindow)
    return m_mainWindow->getContextBar();
  else
    return nullptr;
}

Timeline* App::timeline() const
{
  if (m_mainWindow)
    return m_mainWindow->getTimeline();
  else
    return nullptr;
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
    title += docView->document()->name();
    title += " - ";
  }

  title += defaultTitle;
  she::instance()->defaultDisplay()->setTitleBar(title);
}

InputChain& App::inputChain()
{
  return m_modules->m_inputChain;
}

// Updates palette and redraw the screen.
void app_refresh_screen()
{
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Site site = context->activeSite();

  if (Palette* pal = site.palette())
    set_current_palette(pal, false);
  else
    set_current_palette(NULL, false);

  // Invalidate the whole screen.
  ui::Manager::getDefault()->invalidate();
}

// TODO remove app_rebuild_documents_tabs() and replace it by
// observable events in the document (so a tab can observe if the
// document is modified).
void app_rebuild_documents_tabs()
{
  if (App::instance()->isGui()) {
    App::instance()->workspace()->updateTabs();
    App::instance()->updateDisplayTitleBar();
  }
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
