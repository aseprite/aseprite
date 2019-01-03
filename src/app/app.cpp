// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"

#include "app/app_mod.h"
#include "app/check_update.h"
#include "app/cli/app_options.h"
#include "app/cli/cli_processor.h"
#include "app/cli/default_cli_delegate.h"
#include "app/cli/preview_cli_delegate.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/crash/data_recovery.h"
#include "app/extensions.h"
#include "app/file/file.h"
#include "app/file/file_formats_manager.h"
#include "app/file_system.h"
#include "app/gui_xml.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/log.h"
#include "app/modules.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "app/site.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/backup_indicator.h"
#include "app/ui/color_bar.h"
#include "app/ui/doc_view.h"
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
#include "base/exception.h"
#include "base/fs.h"
#include "base/scoped_lock.h"
#include "base/split_string.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "render/render.h"
#include "os/display.h"
#include "os/error.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <iostream>
#include <memory>

#ifdef ENABLE_SCRIPTING
  #include "app/script/engine.h"
  #include "app/shell.h"
#endif

#ifdef ENABLE_STEAM
  #include "steam/steam.h"
#endif

namespace app {

using namespace ui;

#ifdef ENABLE_SCRIPTING

namespace {

class ConsoleEngineDelegate : public script::EngineDelegate {
public:
  void onConsolePrint(const char* text) override {
    m_console.printf("%s\n", text);
  }
private:
  Console m_console;
};

} // anonymous namespace

#endif // ENABLER_SCRIPTING

class App::CoreModules {
public:
  ConfigModule m_configModule;
  Preferences m_preferences;
};

class App::LoadLanguage {
public:
  LoadLanguage(Preferences& pref,
               Extensions& exts) {
    Strings::createInstance(pref, exts);
  }
};

class App::Modules {
public:
#ifdef ENABLE_UI
  typedef app::UIContext ContextT;
#else
  typedef app::Context ContextT;
#endif

  LoggerModule m_loggerModule;
  FileSystemModule m_file_system_module;
  Extensions m_extensions;
  // Load main language (after loading the extensions)
  LoadLanguage m_loadLanguage;
#ifdef ENABLE_UI
  tools::ToolBox m_toolbox;
  tools::ActiveToolManager m_activeToolManager;
#endif
  Commands m_commands;
  ContextT m_context;
#ifdef ENABLE_UI
  RecentFiles m_recent_files;
  InputChain m_inputChain;
  clipboard::ClipboardManager m_clipboardManager;
#endif
  // This is a raw pointer because we want to delete this explicitly.
  app::crash::DataRecovery* m_recovery;

  Modules(const bool createLogInDesktop,
          Preferences& pref)
    : m_loggerModule(createLogInDesktop)
    , m_loadLanguage(pref, m_extensions)
#ifdef ENABLE_UI
    , m_activeToolManager(&m_toolbox)
    , m_recent_files(pref.general.recentItems())
#endif
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
    m_recovery = new app::crash::DataRecovery(&m_context);
#endif
  }

  void deleteDataRecovery() {
#ifdef ENABLE_DATA_RECOVERY
    delete m_recovery;
    m_recovery = nullptr;
#endif
  }

};

App* App::m_instance = nullptr;

App::App(AppMod* mod)
  : m_mod(mod)
  , m_coreModules(nullptr)
  , m_modules(nullptr)
  , m_legacy(nullptr)
  , m_isGui(false)
  , m_isShell(false)
#ifdef ENABLE_UI
  , m_backupIndicator(nullptr)
#endif
#ifdef ENABLE_SCRIPTING
  , m_engine(new script::Engine)
#endif
{
  ASSERT(m_instance == NULL);
  m_instance = this;
}

void App::initialize(const AppOptions& options)
{
#ifdef ENABLE_UI
  m_isGui = options.startUI() && !options.previewCLI();
#else
  m_isGui = false;
#endif
  m_isShell = options.startShell();
  m_coreModules = new CoreModules;

#ifdef _WIN32
  if (options.disableWintab() ||
      !preferences().experimental.loadWintabDriver()) {
    os::instance()->useWintabAPI(false);
  }
#endif

  if (m_isGui)
    m_uiSystem.reset(new ui::UISystem);

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

  // Load modules
  m_modules = new Modules(createLogInDesktop, preferences());
  m_legacy = new LegacyModules(isGui() ? REQUIRE_INTERFACE: 0);
#ifdef ENABLE_UI
  m_brushes.reset(new AppBrushes);
#endif

  // Data recovery is enabled only in GUI mode
  if (isGui() && preferences().general.dataRecovery())
    m_modules->createDataRecovery();

  if (isPortable())
    LOG("APP: Running in portable mode\n");

  // Load or create the default palette, or migrate the default
  // palette from an old format palette to the new one, etc.
  load_default_palette();

#ifdef ENABLE_UI
  // Initialize GUI interface
  if (isGui()) {
    LOG("APP: GUI mode\n");

    // Set the ClipboardDelegate impl to copy/paste text in the native
    // clipboard from the ui::Entry control.
    m_uiSystem->setClipboardDelegate(&m_modules->m_clipboardManager);

    // Setup the GUI cursor and redraw screen
    ui::set_use_native_cursors(preferences().cursor.useNativeCursor());
    ui::set_mouse_cursor_scale(preferences().cursor.cursorScale());
    ui::set_mouse_cursor(kArrowCursor);

    ui::Manager::getDefault()->invalidate();

    // Create the main window.
    m_mainWindow.reset(new MainWindow);
    if (m_mod)
      m_mod->modMainWindow(m_mainWindow.get());

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
#endif  // ENABLE_UI

  // Process options
  LOG("APP: Processing options...\n");
  {
    std::unique_ptr<CliDelegate> delegate;
    if (options.previewCLI())
      delegate.reset(new PreviewCliDelegate);
    else
      delegate.reset(new DefaultCliDelegate);

    CliProcessor cli(delegate.get(), options);
    cli.process(&m_modules->m_context);
  }

  os::instance()->finishLaunching();
}

void App::run()
{
#ifdef ENABLE_UI
  // Run the GUI
  if (isGui()) {
#ifdef _WIN32
    // How to interpret one finger on Windows tablets.
    ui::Manager::getDefault()->getDisplay()
      ->setInterpretOneFingerGestureAsMouseMovement(
        Preferences::instance().experimental.oneFingerAsMouseMovement());
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
    // Setup app icon for Linux window managers
    try {
      os::Display* display = os::instance()->defaultDisplay();
      os::SurfaceList icons;

      for (const int size : { 32, 64, 128 }) {
        ResourceFinder rf;
        rf.includeDataDir(fmt::format("icons/ase{0}.png", size).c_str());
        if (rf.findFirst()) {
          os::Surface* surf = os::instance()->loadRgbaSurface(rf.filename().c_str());
          if (surf)
            icons.push_back(surf);
        }
      }

      display->setIcons(icons);

      for (auto surf : icons)
        surf->dispose();
    }
    catch (const std::exception&) {
      // Just ignore the exception, we couldn't change the app icon, no
      // big deal.
    }
#endif

    // Initialize Steam API
#ifdef ENABLE_STEAM
    steam::SteamAPI steam;
    if (steam.initialized())
      os::instance()->activateApp();
#endif

#if ENABLE_DEVMODE
    // On OS X, when we compile Aseprite on devmode, we're using it
    // outside an app bundle, so we must active the app explicitly.
    os::instance()->activateApp();
#endif

#ifdef ENABLE_UPDATER
    // Launch the thread to check for updates.
    app::CheckUpdateThreadLauncher checkUpdate(
      m_mainWindow->getCheckUpdateDelegate());
    checkUpdate.launch();
#endif

    app::SendCrash sendCrash;
    sendCrash.search();

    // Keep the console alive the whole program execute (just in case
    // we've to print errors).
    Console console;
#ifdef ENABLE_SCRIPTING
    // Use the app::Console() for script erros
    ConsoleEngineDelegate delegate;
    script::ScopedEngineDelegate setEngineDelegate(m_engine.get(), &delegate);
#endif

    // Run the GUI main message loop
    ui::Manager::getDefault()->run();
  }
#endif  // ENABLE_UI

#ifdef ENABLE_SCRIPTING
  // Start shell to execute scripts.
  if (m_isShell) {
    m_engine->printLastResult(); // TODO is this needed?
    Shell shell;
    shell.run(*m_engine);
  }
#endif  // ENABLE_SCRIPTING

  // Destroy all documents in the UIContext.
  const Docs& docs = m_modules->m_context.documents();
  while (!docs.empty()) {
    Doc* doc = docs.back();

    // First we close the document. In this way we receive recent
    // notifications related to the document as a app::Doc. If
    // we delete the document directly, we destroy the app::Doc
    // too early, and then doc::~Document() call
    // DocsObserver::onRemoveDocument(). In this way, observers
    // could think that they have a fully created app::Doc when
    // in reality it's a doc::Document (in the middle of a
    // destruction process).
    //
    // TODO: This problem is because we're extending doc::Document,
    // in the future, we should remove app::Doc.
    doc->close();
    delete doc;
  }

#ifdef ENABLE_UI
  if (isGui()) {
    // Destroy the window.
    m_mainWindow.reset(NULL);
  }
#endif

  // Delete backups (this is a normal shutdown, we are not handling
  // exceptions, and we are not in a destructor).
  m_modules->deleteDataRecovery();
}

// Finishes the Aseprite application.
App::~App()
{
  try {
    LOG("APP: Exit\n");
    ASSERT(m_instance == this);

#ifdef ENABLE_SCRIPTING
    // Destroy scripting engine
    m_engine.reset(nullptr);
#endif

    // Delete file formats.
    FileFormatsManager::destroyInstance();

    // Fire App Exit signal.
    App::instance()->Exit();

#ifdef ENABLE_UI
    // Finalize modules, configuration and core.
    Editor::destroyEditorSharedInternals();

    if (m_backupIndicator) {
      delete m_backupIndicator;
      m_backupIndicator = nullptr;
    }

    // Save brushes
    m_brushes.reset(nullptr);
#endif

    delete m_legacy;
    delete m_modules;
    delete m_coreModules;

#ifdef ENABLE_UI
    // Destroy the loaded gui.xml data.
    delete KeyboardShortcuts::instance();
    delete GuiXml::instance();
#endif

    m_instance = NULL;
  }
  catch (const std::exception& e) {
    LOG(ERROR) << "APP: Error: " << e.what() << "\n";
    os::error_message(e.what());

    // no re-throw
  }
  catch (...) {
    os::error_message("Error closing " PACKAGE ".\n(uncaught exception)");

    // no re-throw
  }
}

Context* App::context()
{
  return &m_modules->m_context;
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
#ifdef ENABLE_UI
  return &m_modules->m_toolbox;
#else
  return nullptr;
#endif
}

tools::Tool* App::activeTool() const
{
#ifdef ENABLE_UI
  return m_modules->m_activeToolManager.activeTool();
#else
  return nullptr;
#endif
}

tools::ActiveToolManager* App::activeToolManager() const
{
#ifdef ENABLE_UI
  return &m_modules->m_activeToolManager;
#else
  return nullptr;
#endif
}

RecentFiles* App::recentFiles() const
{
#ifdef ENABLE_UI
  ASSERT(m_modules != NULL);
  return &m_modules->m_recent_files;
#else
  return nullptr;
#endif
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

Extensions& App::extensions() const
{
  return m_modules->m_extensions;
}

crash::DataRecovery* App::dataRecovery() const
{
  return m_modules->recovery();
}

#ifdef ENABLE_UI
void App::showNotification(INotificationDelegate* del)
{
  m_mainWindow->showNotification(del);
}

void App::showBackupNotification(bool state)
{
  assert_ui_thread();
  if (state) {
    if (!m_backupIndicator)
      m_backupIndicator = new BackupIndicator;
    m_backupIndicator->start();
  }
  else {
    if (m_backupIndicator)
      m_backupIndicator->stop();
  }
}

void App::updateDisplayTitleBar()
{
  std::string defaultTitle = PACKAGE " v" VERSION;
  std::string title;

  DocView* docView = UIContext::instance()->activeView();
  if (docView) {
    // Prepend the document's filename.
    title += docView->document()->name();
    title += " - ";
  }

  title += defaultTitle;
  os::instance()->defaultDisplay()->setTitleBar(title);
}

InputChain& App::inputChain()
{
  return m_modules->m_inputChain;
}
#endif

void app_update_current_palette()
{
#ifdef ENABLE_UI
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Site site = context->activeSite();

  if (Palette* pal = site.palette())
    set_current_palette(pal, false);
  else
    set_current_palette(nullptr, false);
#endif // ENABLE_UI
}

// Updates palette and redraw the screen.
void app_refresh_screen()
{
#ifdef ENABLE_UI
  app_update_current_palette();

  // Invalidate the whole screen.
  ui::Manager::getDefault()->invalidate();
#endif // ENABLE_UI
}

// TODO remove app_rebuild_documents_tabs() and replace it by
// observable events in the document (so a tab can observe if the
// document is modified).
void app_rebuild_documents_tabs()
{
#ifdef ENABLE_UI
  if (App::instance()->isGui()) {
    App::instance()->workspace()->updateTabs();
    App::instance()->updateDisplayTitleBar();
  }
#endif // ENABLE_UI
}

PixelFormat app_get_current_pixel_format()
{
#ifdef ENABLE_UI
  Context* context = UIContext::instance();
  ASSERT(context != NULL);

  Doc* document = context->activeDocument();
  if (document != NULL)
    return document->sprite()->pixelFormat();
  else
    return IMAGE_RGB;
#else // ENABLE_UI
  return IMAGE_RGB;
#endif
}

void app_default_statusbar_message()
{
#ifdef ENABLE_UI
  StatusBar::instance()
    ->setStatusText(250, "%s %s | %s", PACKAGE, VERSION, COPYRIGHT);
#endif
}

int app_get_color_to_clear_layer(Layer* layer)
{
  ASSERT(layer != NULL);

  app::Color color;

  // The `Background' is erased with the `Background Color'
  if (layer->isBackground()) {
#ifdef ENABLE_UI
    if (ColorBar::instance())
      color = ColorBar::instance()->getBgColor();
    else
#endif
      color = app::Color::fromRgb(0, 0, 0); // TODO get background color color from doc::Settings
  }
  else // All transparent layers are cleared with the mask color
    color = app::Color::fromMask();

  return color_utils::color_for_layer(color, layer);
}

std::string memory_dump_filename()
{
#ifdef _WIN32
  static const char* kDefaultCrashName = PACKAGE "-crash-" VERSION ".dmp";

  app::ResourceFinder rf;
  rf.includeUserDir(kDefaultCrashName);
  return rf.getFirstOrCreateDefault();

#else
  return "";
#endif
}

} // namespace app
