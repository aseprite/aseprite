// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
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
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/crash/data_recovery.h"
#include "app/drm.h"
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
#include "base/platform.h"
#include "base/replace_string.h"
#include "base/split_string.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "os/error.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "render/render.h"
#include "ui/intern.h"
#include "ui/ui.h"
#include "updater/user_agent.h"
#include "ver/info.h"

#if LAF_MACOS
  #include "os/osx/system.h"
#elif LAF_LINUX
  #include "os/x11/system.h"
#endif

#include <iostream>
#include <memory>

#ifdef ENABLE_SCRIPTING
  #include "app/script/engine.h"
  #include "app/shell.h"
#endif

#ifdef ENABLE_STEAM
  #include "steam/steam.h"
#endif

#include <memory>
#include <optional>

namespace app {

using namespace ui;

#ifdef ENABLE_SCRIPTING

namespace {

class ConsoleEngineDelegate : public script::EngineDelegate {
public:
  ConsoleEngineDelegate(Console& console) : m_console(console) {}
  void onConsoleError(const char* text) override { onConsolePrint(text); }
  void onConsolePrint(const char* text) override { m_console.printf("%s\n", text); }

private:
  Console& m_console;
};

} // anonymous namespace

#endif // ENABLER_SCRIPTING

class App::CoreModules {
public:
  ConfigModule m_configModule;
  app::UIContext m_context;
};

class App::LoadLanguage {
public:
  LoadLanguage(Preferences& pref, Extensions& exts) { Strings::createInstance(pref, exts); }
};

class App::Modules {
public:
  LoggerModule m_loggerModule;
  FileSystemModule m_file_system_module;
  Extensions m_extensions;
  // Load main language (after loading the extensions)
  LoadLanguage m_loadLanguage;
  tools::ToolBox m_toolbox;
  tools::ActiveToolManager m_activeToolManager;
  Commands m_commands;
  RecentFiles m_recent_files;
  InputChain m_inputChain;
  Clipboard m_clipboard;
#ifdef ENABLE_DATA_RECOVERY
  // This is a raw pointer because we want to delete it explicitly.
  // (e.g. if an exception occurs, the ~Modules() doesn't have to
  // delete m_recovery)
  std::unique_ptr<app::crash::DataRecovery> m_recovery;
#endif

  Modules(const bool createLogInDesktop, Preferences& pref)
    : m_loggerModule(createLogInDesktop)
    , m_loadLanguage(pref, m_extensions)
    , m_activeToolManager(&m_toolbox)
    , m_recent_files(pref.general.recentItems())
#ifdef ENABLE_DATA_RECOVERY
    , m_recovery(nullptr)
#endif
  {
  }

  ~Modules()
  {
#ifdef ENABLE_DATA_RECOVERY
    ASSERT(m_recovery == nullptr || ui::get_app_state() == ui::AppState::kClosingWithException);
#endif
  }

  app::crash::DataRecovery* recovery()
  {
#ifdef ENABLE_DATA_RECOVERY
    return m_recovery.get();
#else
    return nullptr;
#endif
  }

  void createDataRecovery(Context* ctx)
  {
#ifdef ENABLE_DATA_RECOVERY

  #ifdef ENABLE_TRIAL_MODE
    DRM_INVALID
    {
      return;
    }
  #endif

    m_recovery = std::make_unique<app::crash::DataRecovery>(ctx);
    m_recovery->SessionsListIsReady.connect([] {
      ui::assert_ui_thread();
      auto app = App::instance();
      if (app && app->mainWindow()) {
        // Notify that the list of sessions is ready.
        app->mainWindow()->dataRecoverySessionsAreReady();
      }
    });
#endif
  }

  void searchDataRecoverySessions()
  {
#ifdef ENABLE_DATA_RECOVERY

  #ifdef ENABLE_TRIAL_MODE
    DRM_INVALID
    {
      return;
    }
  #endif

    ASSERT(m_recovery);
    if (m_recovery)
      m_recovery->launchSearch();
#endif
  }

  void deleteDataRecovery()
  {
#ifdef ENABLE_DATA_RECOVERY

  #ifdef ENABLE_TRIAL_MODE
    DRM_INVALID
    {
      return;
    }
  #endif

    m_recovery.reset();
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
  , m_backupIndicator(nullptr)
#ifdef ENABLE_SCRIPTING
  , m_engine(new script::Engine)
#endif
{
  ASSERT(m_instance == nullptr);
  m_instance = this;
}

int App::initialize(const AppOptions& options)
{
  os::System* system = os::instance();

  m_isGui = options.startUI() && !options.previewCLI();

  // Notify the scripting engine that we're going to enter to GUI
  // mode, this is useful so we can mark the stdin file handle as
  // closed so no script can hang the program if it tries to read from
  // stdin when the GUI is running.
#ifdef ENABLE_SCRIPTING
  if (m_isGui)
    m_engine->notifyRunningGui();
#endif

  m_isShell = options.startShell();
  m_coreModules = std::make_unique<CoreModules>();

  auto& pref = preferences();

  os::TabletOptions tabletOptions;

#if LAF_WINDOWS

  if (options.disableWintab() || !pref.experimental.loadWintabDriver() ||
      pref.tablet.api() == "pointer") {
    tabletOptions.api = os::TabletAPI::WindowsPointerInput;
  }
  else if (pref.tablet.api() == "wintab_packets") {
    tabletOptions.api = os::TabletAPI::WintabPackets;
  }
  else { // pref.tablet.api() == "wintab"
    tabletOptions.api = os::TabletAPI::Wintab;
  }
  tabletOptions.setCursorFix = pref.tablet.setCursorFix();

#elif LAF_MACOS

  if (!pref.general.osxAsyncView())
    os::osx_set_async_view(false);

#elif LAF_LINUX

  tabletOptions.detectStylusPattern = pref.general.x11StylusId();

#endif

  system->setTabletOptions(tabletOptions);
  system->setAppName(get_app_name());
  system->setAppMode(m_isGui ? os::AppMode::GUI : os::AppMode::CLI);

  if (m_isGui)
    m_uiSystem.reset(new ui::UISystem);

  bool createLogInDesktop = false;
  switch (options.verboseLevel()) {
    case AppOptions::kNoVerbose: base::set_log_level(ERROR); break;
    case AppOptions::kVerbose:   base::set_log_level(INFO); break;
    case AppOptions::kHighlyVerbose:
      base::set_log_level(VERBOSE);
      createLogInDesktop = true;
      break;
  }

  initialize_color_spaces(pref);

#ifdef ENABLE_DRM
  LOG("APP: Initializing DRM...\n");
  app_configure_drm();
#endif

#ifdef ENABLE_STEAM
  if (options.noInApp())
    m_inAppSteam = false;
#endif

  // Load modules
  m_modules = std::make_unique<Modules>(createLogInDesktop, pref);
  m_legacy = std::make_unique<LegacyModules>(isGui() ? REQUIRE_INTERFACE : 0);
  m_brushes = std::make_unique<AppBrushes>();

  // Data recovery is enabled only in GUI mode
  if (isGui() && pref.general.dataRecovery())
    m_modules->createDataRecovery(context());

  if (isPortable())
    LOG("APP: Running in portable mode\n");

  // Load or create the default palette, or migrate the default
  // palette from an old format palette to the new one, etc.
  load_default_palette();

  // Initialize GUI interface
  if (isGui()) {
    LOG("APP: GUI mode\n");

    // Set the ClipboardDelegate impl to copy/paste text in the native
    // clipboard from the ui::Entry control.
    m_uiSystem->setClipboardDelegate(&m_modules->m_clipboard);

    // Setup the GUI cursor and redraw screen
    ui::set_use_native_cursors(pref.cursor.useNativeCursor());
    ui::set_mouse_cursor_scale(pref.cursor.cursorScale());
    ui::set_mouse_cursor(kArrowCursor);

    auto manager = ui::Manager::getDefault();
    manager->invalidate();

    // Create the main window.
    m_mainWindow.reset(new MainWindow);
    m_mainWindow->initialize();
    if (m_mod)
      m_mod->modMainWindow(m_mainWindow.get());

    // Data recovery is enabled only in GUI mode
    if (pref.general.dataRecovery())
      m_modules->searchDataRecoverySessions();

    // Default status of the main window.
    app_rebuild_documents_tabs();
    m_mainWindow->statusBar()->showDefaultText();

    // Show the main window (this is not modal, the code continues)
    m_mainWindow->openWindow();

#if LAF_LINUX // TODO check why this is required and we cannot call
              //      updateAllDisplays() on Linux/X11
    // Redraw the whole screen.
    manager->invalidate();
#else
    // To know the initial manager size we call to
    // Manager::updateAllDisplays(...) so we receive a
    // Manager::onNewDisplayConfiguration() (which will update the
    // bounds of the manager for first time).  This is required so if
    // the OpenFileCommand (called when we're processing the CLI with
    // OpenBatchOfFiles) shows a dialog to open a sequence of files,
    // the dialog is centered correctly to the manager bounds.
    const int scale = Preferences::instance().general.screenScale();
    const bool gpu = Preferences::instance().general.gpuAcceleration();
    manager->updateAllDisplays(scale, gpu);
#endif
  }

#ifdef ENABLE_SCRIPTING
  // Call the init() function from all plugins
  LOG("APP: Initializing scripts...\n");
  extensions().executeInitActions();
#endif

  // Process options
  LOG("APP: Processing options...\n");
  int code;
  {
    std::unique_ptr<CliDelegate> delegate;
    if (options.previewCLI())
      delegate.reset(new PreviewCliDelegate);
    else
      delegate.reset(new DefaultCliDelegate);

    CliProcessor cli(delegate.get(), options);
    code = cli.process(context());
  }

  LOG("APP: Finish launching...\n");
  system->finishLaunching();
  return code;
}

namespace {

struct CloseMainWindow {
  std::unique_ptr<MainWindow>& m_win;
  CloseMainWindow(std::unique_ptr<MainWindow>& win) : m_win(win) {}
  ~CloseMainWindow() { m_win.reset(nullptr); }
};

// Deletes all docs.
struct DeleteAllDocs {
  Context* m_ctx;
  DeleteAllDocs(Context* ctx) : m_ctx(ctx) {}
  ~DeleteAllDocs()
  {
    std::vector<Doc*> docs;

    // Add all documents that were closed in the past, these docs
    // are not part of any context and they are just temporarily in
    // memory just in case the user wants to recover them.
    for (Doc* doc : static_cast<UIContext*>(m_ctx)->getAndRemoveAllClosedDocs())
      docs.push_back(doc);

    // Add documents that are currently opened/in tabs/in the
    // context.
    for (Doc* doc : m_ctx->documents())
      docs.push_back(doc);

    for (Doc* doc : docs) {
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
  }
};

} // anonymous namespace

void App::run()
{
  CloseMainWindow closeMainWindow(m_mainWindow);
  DeleteAllDocs deleteAllDocsAtExit(context());

  // Run the GUI
  if (isGui()) {
    auto manager = ui::Manager::getDefault();
#if LAF_WINDOWS
    // How to interpret one finger on Windows tablets.
    manager->display()->nativeWindow()->setInterpretOneFingerGestureAsMouseMovement(
      preferences().experimental.oneFingerAsMouseMovement());
#endif

#if LAF_LINUX
    // Setup app icon for Linux window managers
    try {
      os::Window* window = os::instance()->defaultWindow();
      os::SurfaceList icons;

      for (const int size : { 32, 64, 128 }) {
        ResourceFinder rf;
        rf.includeDataDir(fmt::format("icons/ase{0}.png", size).c_str());
        if (rf.findFirst()) {
          os::SurfaceRef surf = os::instance()->loadRgbaSurface(rf.filename().c_str());
          if (surf) {
            surf->setImmutable();
            icons.push_back(surf);
          }
        }
      }

      window->setIcons(icons);
    }
    catch (const std::exception&) {
      // Just ignore the exception, we couldn't change the app icon, no
      // big deal.
    }
#endif

    // Initialize Steam API
#ifdef ENABLE_STEAM
    std::unique_ptr<steam::SteamAPI> steam;
    if (m_inAppSteam) {
      steam = std::make_unique<steam::SteamAPI>();
      if (steam->isInitialized())
        os::instance()->activateApp();
    }
    else {
      // We tried to load the Steam SDK without calling
      // SteamAPI_InitSafe() to check if we could run the program
      // without "in game" indication but still capturing screenshots
      // on Steam, and that wasn't the case.
    }
#endif

#if defined(_DEBUG) || defined(ENABLE_DEVMODE)
    // On OS X, when we compile Aseprite on devmode, we're using it
    // outside an app bundle, so we must active the app explicitly.
    if (isGui())
      os::instance()->activateApp();
#endif

#ifdef ENABLE_UPDATER
    // Launch the thread to check for updates.
    app::CheckUpdateThreadLauncher checkUpdate(m_mainWindow->getCheckUpdateDelegate());
    checkUpdate.launch();
#endif

#if !ENABLE_SENTRY
    app::SendCrash sendCrash;
    sendCrash.search();
#endif

    // Keep the console alive the whole program execute (just in case
    // we've to print errors).
    Console console;
#ifdef ENABLE_SCRIPTING
    // Use the app::Console() for script errors
    ConsoleEngineDelegate delegate(console);
    script::ScopedEngineDelegate setEngineDelegate(m_engine.get(), &delegate);
#endif

    // Run the GUI main message loop
    try {
      manager->run();
      set_app_state(AppState::kClosing);
    }
    catch (...) {
      set_app_state(AppState::kClosingWithException);
      throw;
    }
  }

#ifdef ENABLE_SCRIPTING
  // Start shell to execute scripts.
  if (m_isShell) {
    m_engine->printLastResult(); // TODO is this needed?
    Shell shell;
    shell.run(*m_engine);
  }
#endif // ENABLE_SCRIPTING

  // ----------------------------------------------------------------------

#ifdef ENABLE_SCRIPTING
  // Call the exit() function from all plugins
  extensions().executeExitActions();
#endif

  close();
}

void App::close()
{
  if (isGui()) {
    ExitGui();

    // Select no document
    static_cast<UIContext*>(context())->setActiveView(nullptr);

    // Delete backups (this is a normal shutdown, we are not handling
    // exceptions, and we are not in a destructor).
    m_modules->deleteDataRecovery();
  }
}

// Finishes the Aseprite application.
App::~App()
{
  try {
    LOG("APP: Exit\n");
    ASSERT(m_instance == this);

#ifdef ENABLE_SCRIPTING
    // Destroy scripting engine calling a method (instead of using
    // reset()) because we need to keep the "m_engine" pointer valid
    // until the very end, just in case that some Lua error happens
    // now and we have to print that error using
    // App::instance()->scriptEngine() in some way. E.g. if a Dialog
    // onclose event handler fails with a Lua error when we are
    // closing the app, a Lua error must be printed, and we need a
    // valid m_engine pointer.
    m_engine->destroy();
    m_engine.reset();
#endif

    // Delete file formats.
    FileFormatsManager::destroyInstance();

    // Fire App Exit signal.
    App::instance()->Exit();

    // Finalize modules, configuration and core.
    Editor::destroyEditorSharedInternals();

    m_backupIndicator.reset();

    // Save brushes
    m_brushes.reset();

    m_legacy.reset();
    m_modules.reset();

    // Save preferences only if we are running in GUI mode.  when we
    // run in batch mode we might want to reset some preferences so
    // the scripts have a reproducible behavior. Those reset
    // preferences must not be saved.
    if (isGui())
      preferences().save();

    m_coreModules.reset();

    // Destroy the loaded gui.xml data.
    KeyboardShortcuts::destroyInstance();
    GuiXml::destroyInstance();
  }
  catch (const std::exception& e) {
    LOG(ERROR, "APP: Error: %s\n", e.what());
    os::error_message(e.what());

    // no re-throw
  }
  catch (...) {
    os::error_message("Error closing the program.\n(uncaught exception)");

    // no re-throw
  }

  m_instance = nullptr;
}

Context* App::context()
{
  return &m_coreModules->m_context;
}

bool App::isPortable()
{
  static std::optional<bool> is_portable;
  if (!is_portable) {
    is_portable = base::is_file(
      base::join_path(base::get_file_path(base::get_app_path()), "aseprite.ini"));
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
  ASSERT(m_modules != nullptr);
  return &m_modules->m_recent_files;
}

Workspace* App::workspace() const
{
  if (m_mainWindow)
    return m_mainWindow->getWorkspace();
  return nullptr;
}

ContextBar* App::contextBar() const
{
  if (m_mainWindow)
    return m_mainWindow->getContextBar();
  return nullptr;
}

Timeline* App::timeline() const
{
  if (m_mainWindow)
    return m_mainWindow->getTimeline();
  return nullptr;
}

Preferences& App::preferences() const
{
  return m_coreModules->m_context.preferences();
}

Extensions& App::extensions() const
{
  return m_modules->m_extensions;
}

crash::DataRecovery* App::dataRecovery() const
{
  return m_modules->recovery();
}

void App::showNotification(INotificationDelegate* del)
{
  if (m_mainWindow)
    m_mainWindow->showNotification(del);
}

void App::showBackupNotification(bool state)
{
  assert_ui_thread();
  if (state) {
    if (!m_backupIndicator)
      m_backupIndicator = std::make_unique<BackupIndicator>();
    m_backupIndicator->start();
  }
  else {
    if (m_backupIndicator)
      m_backupIndicator->stop();
  }
}

void App::updateDisplayTitleBar()
{
  static std::string defaultTitle;
  std::string title;

  if (defaultTitle.empty()) {
    defaultTitle = fmt::format("{} v{}", get_app_name(), get_app_version());

#if LAF_MACOS
    // On macOS we remove the "-arm64" suffix for Apple Silicon as it
    // will be the most common platform from now on.
    if constexpr (base::Platform::arch == base::Platform::Arch::arm64) {
      base::replace_string(defaultTitle, "-arm64", "");
    }
    else if constexpr (base::Platform::arch == base::Platform::Arch::x64) {
      base::replace_string(defaultTitle, "-x64", "");
      defaultTitle += " (x64)";
    }
#else
    // On PC (Windows/Linux) we try to remove "-x64" suffix as it's
    // the most common platform.
    if constexpr (base::Platform::arch == base::Platform::Arch::x64) {
      base::replace_string(defaultTitle, "-x64", "");
    }
    else if constexpr (base::Platform::arch == base::Platform::Arch::x86) {
      base::replace_string(defaultTitle, "-x86", "");
      defaultTitle += " (x86)";
    }
#endif
  }

  DocView* docView = UIContext::instance()->activeView();
  if (docView) {
    // Prepend the document's filename.
    title += docView->document()->name();
    if (docView->document()->isReadOnly()) {
      title += " [Read-Only]";
    }
    title += " - ";
  }

  title += defaultTitle;
  os::instance()->defaultWindow()->setTitle(title);
}

InputChain& App::inputChain()
{
  return m_modules->m_inputChain;
}

// Updates palette and redraw the screen.
void app_refresh_screen()
{
  Context* ctx = UIContext::instance();
  ASSERT(ctx != nullptr);
  if (!ctx)
    return;

  Site site = ctx->activeSite();
  if (Palette* pal = site.palette())
    set_current_palette(pal, false);
  else
    set_current_palette(nullptr, false);

  // Invalidate the whole screen.
  if (auto* man = ui::Manager::getDefault())
    man->invalidate();
}

// TODO remove app_rebuild_documents_tabs() and replace it by
// observable events in the document (so a tab can observe if the
// document is modified).
void app_rebuild_documents_tabs()
{
  if (auto* app = App::instance(); app->isGui()) {
    app->workspace()->updateTabs();
    app->updateDisplayTitleBar();
  }
}

PixelFormat app_get_current_pixel_format()
{
  Context* ctx = App::instance()->context();
  ASSERT(ctx);

  Doc* doc = ctx->activeDocument();
  if (doc)
    return doc->sprite()->pixelFormat();
  return IMAGE_RGB;
}

int app_get_color_to_clear_layer(Layer* layer)
{
  ASSERT(layer != nullptr);

  app::Color color;

  // The `Background' is erased with the `Background Color'
  if (layer->isBackground()) {
    if (auto* colorBar = ColorBar::instance())
      color = colorBar->getBgColor();
    else
      color = Preferences::instance().colorBar.bgColor();
  }
  else { // All transparent layers are cleared with the mask color
    color = app::Color::fromMask();
  }

  return color_utils::color_for_layer(color, layer);
}

#ifdef ENABLE_DRM
void app_configure_drm()
{
  ResourceFinder userDirRf, dataDirRf;
  userDirRf.includeUserDir("");
  dataDirRf.includeDataDir("");
  std::map<std::string, std::string> config = {
    { "data", dataDirRf.getFirstOrCreateDefault() }
  };
  DRM_CONFIGURE(get_app_url(),
                get_app_name(),
                get_app_version(),
                userDirRf.getFirstOrCreateDefault(),
                updater::getUserAgent(),
                config);
}
#endif

} // namespace app
