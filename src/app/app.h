// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_H_INCLUDED
#define APP_APP_H_INCLUDED
#pragma once

#ifdef ENABLE_UI
#include "app/app_brushes.h"
#endif

#include "base/paths.h"
#include "doc/pixel_format.h"
#include "obs/signal.h"

#include <memory>
#include <string>
#include <vector>

namespace doc {
  class Layer;
}

namespace ui {
  class UISystem;
}

namespace app {

#ifdef ENABLE_SCRIPTING
  namespace script {
    class Engine;
  }
#endif

  class AppMod;
  class AppOptions;
  class BackupIndicator;
  class Context;
  class ContextBar;
  class Doc;
  class Extensions;
  class INotificationDelegate;
  class InputChain;
  class LegacyModules;
  class LoggerModule;
  class MainWindow;
  class Preferences;
  class RecentFiles;
  class Timeline;
  class Workspace;

  namespace crash {
    class DataRecovery;
  }

  namespace tools {
    class ActiveToolManager;
    class Tool;
    class ToolBox;
  }

  using namespace doc;

  class App {
  public:
    App(AppMod* mod = nullptr);
    ~App();

    static App* instance() { return m_instance; }

    Context* context();

    // Returns true if Aseprite is running with GUI available.
    bool isGui() const { return m_isGui; }

    // Returns true if the application is running in portable mode.
    bool isPortable();

    // Runs the Aseprite application. In GUI mode it's the top-level
    // window, in console/scripting it just runs the specified
    // scripts.
    int initialize(const AppOptions& options);
    void run();
    void close();

    AppMod* mod() const { return m_mod; }
    tools::ToolBox* toolBox() const;
    tools::Tool* activeTool() const;
    tools::ActiveToolManager* activeToolManager() const;
    RecentFiles* recentFiles() const;
    MainWindow* mainWindow() const { return m_mainWindow.get(); }
    Workspace* workspace() const;
    ContextBar* contextBar() const;
    Timeline* timeline() const;
    Preferences& preferences() const;
    Extensions& extensions() const;
    crash::DataRecovery* dataRecovery() const;

#ifdef ENABLE_UI
    AppBrushes& brushes() {
      ASSERT(m_brushes.get());
      return *m_brushes;
    }

    void showNotification(INotificationDelegate* del);
    void showBackupNotification(bool state);
    void updateDisplayTitleBar();

    InputChain& inputChain();
#endif

#ifdef ENABLE_SCRIPTING
    script::Engine* scriptEngine() { return m_engine.get(); }
#endif

    const std::string& memoryDumpFilename() const { return m_memoryDumpFilename; }
    void memoryDumpFilename(const std::string& fn) { m_memoryDumpFilename = fn; }

    // App Signals
    obs::signal<void()> Exit;
    obs::signal<void()> PaletteChange;
    obs::signal<void()> ColorSpaceChange;
    obs::signal<void()> PalettePresetsChange;

  private:
    class CoreModules;
    class LoadLanguage;
    class Modules;

    static App* m_instance;

    AppMod* m_mod;
    std::unique_ptr<ui::UISystem> m_uiSystem;
    std::unique_ptr<CoreModules> m_coreModules;
    std::unique_ptr<Modules> m_modules;
    std::unique_ptr<LegacyModules> m_legacy;
    bool m_isGui;
    bool m_isShell;
    std::unique_ptr<MainWindow> m_mainWindow;
    base::paths m_files;
#ifdef ENABLE_UI
    std::unique_ptr<AppBrushes> m_brushes;
    std::unique_ptr<BackupIndicator> m_backupIndicator;
#endif // ENABLE_UI
#ifdef ENABLE_SCRIPTING
    std::unique_ptr<script::Engine> m_engine;
#endif

    // Set the memory dump filename to show in the Preferences dialog
    // or the "send crash" dialog. It's set by the SendCrash class.
    std::string m_memoryDumpFilename;
  };

  void app_refresh_screen();
  void app_rebuild_documents_tabs();
  PixelFormat app_get_current_pixel_format();
  int app_get_color_to_clear_layer(doc::Layer* layer);
  void app_configure_drm();

} // namespace app

#endif
