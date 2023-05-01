// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/cmd_open_file.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/crash/data_recovery.h"
#include "app/doc.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/ink.h"
#include "app/tools/tool_box.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "app/util/open_batch.h"
#include "base/fs.h"
#include "base/memory.h"
#include "base/string.h"
#include "doc/sprite.h"
#include "os/error.h"
#include "os/screen.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/intern.h"
#include "ui/ui.h"

#ifdef ENABLE_STEAM
  #include "steam/steam.h"
#endif

#include <algorithm>
#include <cstdio>
#include <list>
#include <vector>

#if defined(ENABLE_DEVMODE) && defined(ENABLE_DATA_RECOVERY)
  #include "app/crash/data_recovery.h"
#endif

namespace app {

using namespace gfx;
using namespace ui;
using namespace app::skin;

static struct {
  int width;
  int height;
  int scale;
} try_resolutions[] = { { 1024, 768, 2 },
                        {  800, 600, 2 },
                        {  640, 480, 2 },
                        {  320, 240, 1 },
                        {  320, 200, 1 },
                        {    0,   0, 0 } };

//////////////////////////////////////////////////////////////////////

class CustomizedGuiManager : public ui::Manager
                           , public ui::LayoutIO {
public:
  CustomizedGuiManager(const os::WindowRef& nativeWindow)
    : ui::Manager(nativeWindow) {
  }

protected:
  bool onProcessMessage(Message* msg) override;
#if ENABLE_DEVMODE
  bool onProcessDevModeKeyDown(KeyMessage* msg);
#endif
  void onInitTheme(InitThemeEvent& ev) override;
  LayoutIO* onGetLayoutIO() override { return this; }
  void onNewDisplayConfiguration(Display* display) override;

  // LayoutIO implementation
  std::string loadLayout(Widget* widget) override;
  void saveLayout(Widget* widget, const std::string& str) override;

private:
  bool processKey(Message* msg);
};

static os::WindowRef main_window = nullptr;
static CustomizedGuiManager* manager = nullptr;
static Theme* gui_theme = nullptr;

static ui::Timer* defered_invalid_timer = nullptr;
static gfx::Region defered_invalid_region;

// Load & save graphics configuration
static bool load_gui_config(os::WindowSpec& spec, bool& maximized);
static void save_gui_config();

static bool create_main_window(bool gpuAccel,
                               bool& maximized,
                               std::string& lastError)
{
  os::WindowSpec spec;
  if (!load_gui_config(spec, maximized))
    return false;

  // Scale is equal to 0 when it's the first time the program is
  // executed.
  int scale = Preferences::instance().general.screenScale();

  os::instance()->setGpuAcceleration(gpuAccel);

  try {
    if (!spec.frame().isEmpty() ||
        !spec.contentRect().isEmpty()) {
      spec.scale(scale == 0 ? 2: std::clamp(scale, 1, 4));
      main_window = os::instance()->makeWindow(spec);
    }
  }
  catch (const os::WindowCreationException& e) {
    lastError = e.what();
  }

  if (!main_window) {
    for (int c=0; try_resolutions[c].width; ++c) {
      try {
        spec.frame();
        spec.position(os::WindowSpec::Position::Default);
        spec.scale(scale == 0 ? try_resolutions[c].scale: scale);
        spec.contentRect(gfx::Rect(0, 0,
                                   try_resolutions[c].width * spec.scale(),
                                   try_resolutions[c].height * spec.scale()));
        main_window = os::instance()->makeWindow(spec);
        break;
      }
      catch (const os::WindowCreationException& e) {
        lastError = e.what();
      }
    }
  }

  if (main_window) {
    // Change the scale value only in the first run (this will be
    // saved when the program is closed).
    if (scale == 0)
      Preferences::instance().general.screenScale(main_window->scale());

    if (main_window->isMinimized())
      main_window->maximize();
  }

  return (main_window != nullptr);
}

// Initializes GUI.
int init_module_gui()
{
  auto& pref = Preferences::instance();
  bool maximized = false;
  std::string lastError = "Unknown error";
  bool gpuAccel = pref.general.gpuAcceleration();

  if (!create_main_window(gpuAccel, maximized, lastError)) {
    // If we've created the native window with hardware acceleration,
    // now we try to do it without hardware acceleration.
    if (gpuAccel &&
        os::instance()->hasCapability(os::Capabilities::GpuAccelerationSwitch)) {
      if (create_main_window(false, maximized, lastError)) {
        // Disable hardware acceleration
        pref.general.gpuAcceleration(false);
      }
    }
  }

  if (!main_window) {
    os::error_message(
      ("Unable to create a user-interface window.\nDetails: "+lastError+"\n").c_str());
    return -1;
  }

  // Create the default-manager
  manager = new CustomizedGuiManager(main_window);

  // Setup the GUI theme for all widgets
  gui_theme = new SkinTheme;
  ui::set_theme(gui_theme, pref.general.uiScale());

  if (maximized)
    main_window->maximize();

  // Handle live resize too redraw the entire manager, dispatch the UI
  // messages, and flip the window.
  os::instance()->handleWindowResize =
    [](os::Window* window) {
      Display* display = Manager::getDisplayFromNativeWindow(window);
      if (!display)
        display = manager->display();
      ASSERT(display);

      Message* msg = new Message(kResizeDisplayMessage);
      msg->setDisplay(display);
      msg->setRecipient(manager);
      msg->setPropagateToChildren(false);

      manager->enqueueMessage(msg);
      manager->dispatchMessages();
    };

  // Set graphics options for next time
  save_gui_config();

  update_windows_color_profile_from_preferences();

  return 0;
}

void exit_module_gui()
{
  save_gui_config();

  delete defered_invalid_timer;
  delete manager;

  // Now we can destroy theme
  ui::set_theme(nullptr, ui::guiscale());
  delete gui_theme;

  // This should be the last unref() of the display to delete it.
  main_window.reset();
}

void update_windows_color_profile_from_preferences()
{
  auto system = os::instance();

  gen::WindowColorProfile windowProfile;
  if (Preferences::instance().color.manage())
    windowProfile = Preferences::instance().color.windowProfile();
  else
    windowProfile = gen::WindowColorProfile::SRGB;

  os::ColorSpaceRef osCS = nullptr;

  switch (windowProfile) {
    case gen::WindowColorProfile::MONITOR:
      osCS = nullptr;
      break;
    case gen::WindowColorProfile::SRGB:
      osCS = system->makeColorSpace(gfx::ColorSpace::MakeSRGB());
      break;
    case gen::WindowColorProfile::SPECIFIC: {
      std::string name =
        Preferences::instance().color.windowProfileName();

      std::vector<os::ColorSpaceRef> colorSpaces;
      system->listColorSpaces(colorSpaces);

      for (auto& cs : colorSpaces) {
        auto gfxCs = cs->gfxColorSpace();
        if (gfxCs->type() == gfx::ColorSpace::ICC &&
            gfxCs->name() == name) {
          osCS = cs;
          break;
        }
      }
      break;
    }
  }

  // Set the default color space for all windows (osCS can be nullptr
  // which means that each window should use its monitor color space)
  system->setWindowsColorSpace(osCS);

  // Set the color space of all windows
  for (ui::Widget* widget : manager->children()) {
    ASSERT(widget->type() == ui::kWindowWidget);
    auto window = static_cast<ui::Window*>(widget);
    if (window->ownDisplay()) {
      if (auto display = window->display())
        display->nativeWindow()->setColorSpace(osCS);
    }
  }
}

static bool load_gui_config(os::WindowSpec& spec, bool& maximized)
{
  os::ScreenRef screen = os::instance()->mainScreen();
#ifdef LAF_SKIA
  ASSERT(screen);
#else
  // Compiled without Skia (none backend), without screen.
  if (!screen) {
    std::printf(
      "\n"
      "  Aseprite cannot initialize GUI because it was compiled with LAF_BACKEND=none\n"
      "\n"
      "  Check the documentation in:\n"
      "  https://github.com/aseprite/laf/blob/main/README.md\n"
      "  https://github.com/aseprite/aseprite/blob/main/INSTALL.md\n"
      "\n");
    return false;
  }
#endif

  spec.screen(screen);

  gfx::Rect frame;
  frame = get_config_rect("GfxMode", "Frame", frame);
  if (!frame.isEmpty()) {
    spec.position(os::WindowSpec::Position::Frame);

    // Limit the content rect position into the available workarea,
    // e.g. this is needed in case that the user closed Aseprite in a
    // 2nd monitor that then unplugged and start Aseprite again.
    bool ok = false;
    os::ScreenList screens;
    os::instance()->listScreens(screens);
    for (const auto& screen : screens) {
      gfx::Rect wa = screen->workarea();
      gfx::Rect intersection = (frame & wa);
      if (intersection.w >= 32 &&
          intersection.h >= 32) {
        ok = true;
        break;
      }
    }

    // Reset content rect
    if (!ok) {
      spec.position(os::WindowSpec::Position::Default);
      frame = gfx::Rect();
    }
  }

  if (frame.isEmpty()) {
    frame = screen->workarea().shrink(64);

    // Try to get Width/Height from previous Aseprite versions
    frame.w = get_config_int("GfxMode", "Width", frame.w);
    frame.h = get_config_int("GfxMode", "Height", frame.h);
  }
  spec.frame(frame);

  maximized = get_config_bool("GfxMode", "Maximized", true);

  ui::set_multiple_displays(Preferences::instance().experimental.multipleWindows());
  return true;
}

static void save_gui_config()
{
  os::Window* window = manager->display()->nativeWindow();
  if (window) {
    const bool maximized = (window->isMaximized() ||
                            window->isFullscreen());
    const gfx::Rect frame = (maximized ? window->restoredFrame():
                                         window->frame());

    set_config_bool("GfxMode", "Maximized", maximized);
    set_config_rect("GfxMode", "Frame", frame);
  }
}

void update_screen_for_document(const Doc* document)
{
  // Without document.
  if (!document) {
    // Well, change to the default palette.
    if (set_current_palette(NULL, false)) {
      // If the palette changes, refresh the whole screen.
      if (manager)
        manager->invalidate();
    }
  }
  // With a document.
  else {
    const_cast<Doc*>(document)->notifyGeneralUpdate();

    // Update the tabs (maybe the modified status has been changed).
    app_rebuild_documents_tabs();
  }
}

void load_window_pos(Window* window, const char* section,
                     const bool limitMinSize)
{
  Display* parentDisplay =
    (window->display() ? window->display():
                         window->manager()->display());
  Rect workarea =
    (get_multiple_displays() ?
     parentDisplay->nativeWindow()->screen()->workarea():
     parentDisplay->bounds());

  // Default position
  Rect origPos = window->bounds();

  // Load configurated position
  Rect pos = get_config_rect(section, "WindowPos", origPos);

  if (limitMinSize) {
    pos.w = std::clamp(pos.w, origPos.w, workarea.w);
    pos.h = std::clamp(pos.h, origPos.h, workarea.h);
  }
  else {
    pos.w = std::min(pos.w, workarea.w);
    pos.h = std::min(pos.h, workarea.h);
  }

  pos.setOrigin(Point(std::clamp(pos.x, workarea.x, workarea.x2()-pos.w),
                      std::clamp(pos.y, workarea.y, workarea.y2()-pos.h)));

  window->setBounds(pos);

  if (get_multiple_displays()) {
    Rect frame = get_config_rect(section, "WindowFrame", gfx::Rect());
    if (!frame.isEmpty()) {
      limit_with_workarea(parentDisplay, frame);
      window->loadNativeFrame(frame);
    }
  }
  else {
    del_config_value(section, "WindowFrame");
  }
}

void save_window_pos(Window* window, const char* section)
{
  gfx::Rect rc;

  if (!window->lastNativeFrame().isEmpty()) {
    const os::Window* mainNativeWindow = manager->display()->nativeWindow();
    rc = window->lastNativeFrame();
    set_config_rect(section, "WindowFrame", rc);
    rc.offset(-mainNativeWindow->frame().origin());
    rc /= mainNativeWindow->scale();
  }
  else {
    del_config_value(section, "WindowFrame");
    rc = window->bounds();
  }

  set_config_rect(section, "WindowPos", rc);
}

// TODO Replace this with new theme styles
Widget* setup_mini_font(Widget* widget)
{
  auto skinProp = get_skin_property(widget);
  skinProp->setMiniFont();
  return widget;
}

// TODO Replace this with new theme styles
Widget* setup_mini_look(Widget* widget)
{
  auto skinProp = get_skin_property(widget);
  skinProp->setLook(MiniLook);
  return widget;
}

//////////////////////////////////////////////////////////////////////
// Button style (convert radio or check buttons and draw it like
// normal buttons)

void defer_invalid_rect(const gfx::Rect& rc)
{
  if (!defered_invalid_timer)
    defered_invalid_timer = new ui::Timer(250, manager);

  defered_invalid_timer->stop();
  defered_invalid_timer->start();
  defered_invalid_region.createUnion(defered_invalid_region, gfx::Region(rc));
}

//////////////////////////////////////////////////////////////////////
// Manager event handler.

bool CustomizedGuiManager::onProcessMessage(Message* msg)
{
#ifdef ENABLE_STEAM
  if (auto steamAPI = steam::SteamAPI::instance())
    steamAPI->runCallbacks();
#endif

  switch (msg->type()) {

    case kCloseDisplayMessage:
      // Only call the exit command/close the app when the the main
      // display is the closed window in this kCloseDisplayMessage
      // message and it's the current running foreground window.
      if (msg->display() == this->display() &&
          getForegroundWindow() == App::instance()->mainWindow()) {
        // Execute the "Exit" command.
        Command* command = Commands::instance()->byId(CommandId::Exit());
        UIContext::instance()->executeCommandFromMenuOrShortcut(command);
        return true;
      }
      break;

    case kDropFilesMessage:
      // Files are processed only when the main window is the current
      // window running.
      //
      // TODO could we send the files to each dialog?
      if (getForegroundWindow() == App::instance()->mainWindow()) {
        base::paths files = static_cast<DropFilesMessage*>(msg)->files();
        UIContext* ctx = UIContext::instance();
        OpenBatchOfFiles batch;

        while (!files.empty()) {
          auto fn = files.front();
          files.erase(files.begin());

          // If the document is already open, select it.
          Doc* doc = ctx->documents().getByFileName(fn);
          if (doc) {
            DocView* docView = ctx->getFirstDocView(doc);
            if (docView)
              ctx->setActiveView(docView);
            else {
              ASSERT(false);    // Must be some DocView available
            }
          }
          // Load the file
          else {
            // Depending on the file type we will want to do different things:
            std::string extension = base::string_to_lower(
              base::get_file_extension(fn));

            // Install the extension
            if (extension == "aseprite-extension") {
              Command* cmd = Commands::instance()->byId(CommandId::Options());
              Params params;
              params.set("installExtension", fn.c_str());
              ctx->executeCommandFromMenuOrShortcut(cmd, params);
            }
            // Other extensions will be handled as an image/sprite
            else {
              batch.open(ctx, fn,
                         false); // Open all frames

              // Remove all used file names from the "dropped files"
              for (const auto& usedFn : batch.usedFiles()) {
                auto it = std::find(files.begin(), files.end(), usedFn);
                if (it != files.end())
                  files.erase(it);
              }
            }
          }
        }
      }
      break;

    case kKeyDownMessage: {
#if ENABLE_DEVMODE
      if (onProcessDevModeKeyDown(static_cast<KeyMessage*>(msg)))
        return true;
#endif  // ENABLE_DEVMODE

      // Call base impl to check if there is a foreground window as
      // top level that needs keys. (In this way we just do not
      // process keyboard shortcuts for menus and tools).
      if (Manager::onProcessMessage(msg))
        return true;

      if (processKey(msg))
        return true;

      break;
    }

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == defered_invalid_timer) {
        invalidateRegion(defered_invalid_region);
        defered_invalid_region.clear();
        defered_invalid_timer->stop();
      }
      break;

  }

  return Manager::onProcessMessage(msg);
}

#if ENABLE_DEVMODE
bool CustomizedGuiManager::onProcessDevModeKeyDown(KeyMessage* msg)
{
  // Ctrl+Shift+Q generates a crash (useful to test the anticrash feature)
  if (msg->ctrlPressed() &&
      msg->shiftPressed() &&
      msg->scancode() == kKeyQ) {
    int* p = nullptr;
    *p = 0;      // *Crash*
    return true; // This line should not be executed anyway
  }

  // Ctrl+F1 switches screen/UI scaling
  if (msg->ctrlPressed() &&
      msg->scancode() == kKeyF1) {
    try {
      os::Window* window = display()->nativeWindow();
      int screenScale = window->scale();
      int uiScale = ui::guiscale();

      if (msg->shiftPressed()) {
        if (screenScale == 2 && uiScale == 1) {
          screenScale = 1;
          uiScale = 1;
        }
        else if (screenScale == 1 && uiScale == 1) {
          screenScale = 1;
          uiScale = 2;
        }
        else if (screenScale == 1 && uiScale == 2) {
          screenScale = 2;
          uiScale = 1;
        }
      }
      else {
        if (screenScale == 2 && uiScale == 1) {
          screenScale = 1;
          uiScale = 2;
        }
        else if (screenScale == 1 && uiScale == 2) {
          screenScale = 1;
          uiScale = 1;
        }
        else if (screenScale == 1 && uiScale == 1) {
          screenScale = 2;
          uiScale = 1;
        }
      }

      if (uiScale != ui::guiscale()) {
        ui::set_theme(ui::get_theme(), uiScale);
      }
      if (screenScale != window->scale()) {
        updateAllDisplaysWithNewScale(screenScale);
      }
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
    return true;
  }

#ifdef ENABLE_DATA_RECOVERY
  // Ctrl+Shift+R recover active sprite from the backup store
  auto editor = Editor::activeEditor();
  if (msg->ctrlPressed() &&
      msg->shiftPressed() &&
      msg->scancode() == kKeyR &&
      App::instance()->dataRecovery() &&
      App::instance()->dataRecovery()->activeSession() &&
      editor &&
      editor->document()) {
    Doc* doc = App::instance()
      ->dataRecovery()
      ->activeSession()
      ->restoreBackupById(editor->document()->id(), nullptr);
    if (doc)
      UIContext::instance()->documents().add(doc);
    return true;
  }
#endif  // ENABLE_DATA_RECOVERY

  return false;
}
#endif  // ENABLE_DEVMODE

void CustomizedGuiManager::onInitTheme(InitThemeEvent& ev)
{
  Manager::onInitTheme(ev);

  // Update the theme on all menus
  AppMenus::instance()->initTheme();
}

void CustomizedGuiManager::onNewDisplayConfiguration(Display* display)
{
  Manager::onNewDisplayConfiguration(display);

  // Only whne the main display/window is modified
  if (display == this->display()) {
    save_gui_config();

    // TODO Should we provide a more generic way for all ui::Window to
    //      detect the os::Window (or UI Screen Scaling) change?
    Console::notifyNewDisplayConfiguration();
  }
}

bool CustomizedGuiManager::processKey(Message* msg)
{
  App* app = App::instance();
  const KeyboardShortcuts* keys = KeyboardShortcuts::instance();
  const KeyContext contexts[] = {
    keys->getCurrentKeyContext(),
    KeyContext::Normal
  };
  int n = (contexts[0] != contexts[1] ? 2: 1);
  for (int i = 0; i < n; ++i) {
    for (const KeyPtr& key : *keys) {
      if (key->isPressed(msg, *keys, contexts[i])) {
        // Cancel menu-bar loops (to close any popup menu)
        app->mainWindow()->getMenuBar()->cancelMenuLoop();

        switch (key->type()) {

          case KeyType::Tool: {
            tools::Tool* current_tool = app->activeTool();
            tools::Tool* select_this_tool = key->tool();
            tools::ToolBox* toolbox = app->toolBox();
            std::vector<tools::Tool*> possibles;

            // Collect all tools with the pressed keyboard-shortcut
            for (tools::Tool* tool : *toolbox) {
              const KeyPtr key = keys->tool(tool);
              if (key && key->isPressed(msg, *keys))
                possibles.push_back(tool);
            }

            if (possibles.size() >= 2) {
              bool done = false;

              for (size_t i=0; i<possibles.size(); ++i) {
                if (possibles[i] != current_tool &&
                    ToolBar::instance()->isToolVisible(possibles[i])) {
                  select_this_tool = possibles[i];
                  done = true;
                  break;
                }
              }

              if (!done) {
                for (size_t i=0; i<possibles.size(); ++i) {
                  // If one of the possibilities is the current tool
                  if (possibles[i] == current_tool) {
                    // We select the next tool in the possibilities
                    select_this_tool = possibles[(i+1) % possibles.size()];
                    break;
                  }
                }
              }
            }

            ToolBar::instance()->selectTool(select_this_tool);
            return true;
          }

          case KeyType::Command: {
            Command* command = key->command();

            // Commands are executed only when the main window is
            // the current window running.
            if (getForegroundWindow() == app->mainWindow()) {
              // OK, so we can execute the command represented
              // by the pressed-key in the message...
              UIContext::instance()->executeCommandFromMenuOrShortcut(
                command, key->params());
              return true;
            }
            break;
          }

          case KeyType::Quicktool: {
            // Do nothing, it is used in the editor through the
            // KeyboardShortcuts::getCurrentQuicktool() function.
            break;
          }

        }
        break;
      }
    }
  }
  return false;
}

std::string CustomizedGuiManager::loadLayout(Widget* widget)
{
  if (widget->window() == nullptr)
    return "";

  std::string windowId = widget->window()->id();
  std::string widgetId = widget->id();

  return get_config_string(("layout:"+windowId).c_str(), widgetId.c_str(), "");
}

void CustomizedGuiManager::saveLayout(Widget* widget, const std::string& str)
{
  if (widget->window() == NULL)
    return;

  std::string windowId = widget->window()->id();
  std::string widgetId = widget->id();

  set_config_string(("layout:"+windowId).c_str(),
                    widgetId.c_str(),
                    str.c_str());
}

} // namespace app
