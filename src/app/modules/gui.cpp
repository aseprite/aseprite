// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/modules/editors.h"
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
#include "base/clamp.h"
#include "base/fs.h"
#include "base/memory.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "doc/sprite.h"
#include "os/display.h"
#include "os/error.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <algorithm>
#include <list>
#include <vector>

#if defined(_DEBUG) && defined(ENABLE_DATA_RECOVERY)
#include "app/crash/data_recovery.h"
#include "app/modules/editors.h"
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

class CustomizedGuiManager : public Manager
                           , public LayoutIO {
protected:
  bool onProcessMessage(Message* msg) override;
#if ENABLE_DEVMODE
  bool onProcessDevModeKeyDown(KeyMessage* msg);
#endif
  void onInitTheme(InitThemeEvent& ev) override;
  LayoutIO* onGetLayoutIO() override { return this; }
  void onNewDisplayConfiguration() override;

  // LayoutIO implementation
  std::string loadLayout(Widget* widget) override;
  void saveLayout(Widget* widget, const std::string& str) override;
};

static os::Display* main_display = NULL;
static CustomizedGuiManager* manager = NULL;
static Theme* gui_theme = NULL;

static ui::Timer* defered_invalid_timer = nullptr;
static gfx::Region defered_invalid_region;

// Load & save graphics configuration
static void load_gui_config(int& w, int& h, bool& maximized,
                            std::string& windowLayout);
static void save_gui_config();

static bool create_main_display(bool gpuAccel,
                                bool& maximized,
                                std::string& lastError)
{
  int w, h;
  std::string windowLayout;
  load_gui_config(w, h, maximized, windowLayout);

  // Scale is equal to 0 when it's the first time the program is
  // executed.
  int scale = Preferences::instance().general.screenScale();

  os::instance()->setGpuAcceleration(gpuAccel);

  try {
    if (w > 0 && h > 0) {
      main_display = os::instance()->createDisplay(
        w, h, (scale == 0 ? 2: MID(1, scale, 4)));
    }
  }
  catch (const os::DisplayCreationException& e) {
    lastError = e.what();
  }

  if (!main_display) {
    for (int c=0; try_resolutions[c].width; ++c) {
      try {
        main_display =
          os::instance()->createDisplay(
            try_resolutions[c].width,
            try_resolutions[c].height,
            (scale == 0 ? try_resolutions[c].scale: scale));
        break;
      }
      catch (const os::DisplayCreationException& e) {
        lastError = e.what();
      }
    }
  }

  if (main_display) {
    // Change the scale value only in the first run (this will be
    // saved when the program is closed).
    if (scale == 0)
      Preferences::instance().general.screenScale(main_display->scale());

    if (!windowLayout.empty()) {
      main_display->setLayout(windowLayout);
      if (main_display->isMinimized())
        main_display->maximize();
    }
  }

  return (main_display != nullptr);
}

// Initializes GUI.
int init_module_gui()
{
  auto& pref = Preferences::instance();
  bool maximized = false;
  std::string lastError = "Unknown error";
  bool gpuAccel = pref.general.gpuAcceleration();

  if (!create_main_display(gpuAccel, maximized, lastError)) {
    // If we've created the display with hardware acceleration,
    // now we try to do it without hardware acceleration.
    if (gpuAccel &&
        (int(os::instance()->capabilities()) &
         int(os::Capabilities::GpuAccelerationSwitch)) == int(os::Capabilities::GpuAccelerationSwitch)) {
      if (create_main_display(false, maximized, lastError)) {
        // Disable hardware acceleration
        pref.general.gpuAcceleration(false);
      }
    }
  }

  if (!main_display) {
    os::error_message(
      ("Unable to create a user-interface display.\nDetails: "+lastError+"\n").c_str());
    return -1;
  }

  // Create the default-manager
  manager = new CustomizedGuiManager();
  manager->setDisplay(main_display);

  // Setup the GUI theme for all widgets
  gui_theme = new SkinTheme;
  ui::set_theme(gui_theme, pref.general.uiScale());

  if (maximized)
    main_display->maximize();

  // Set graphics options for next time
  save_gui_config();

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

  main_display->dispose();
}

static void load_gui_config(int& w, int& h, bool& maximized,
                            std::string& windowLayout)
{
  gfx::Size defSize = os::instance()->defaultNewDisplaySize();

  w = get_config_int("GfxMode", "Width", defSize.w);
  h = get_config_int("GfxMode", "Height", defSize.h);
  maximized = get_config_bool("GfxMode", "Maximized", false);
  windowLayout = get_config_string("GfxMode", "WindowLayout", "");
}

static void save_gui_config()
{
  os::Display* display = manager->getDisplay();
  if (display) {
    set_config_bool("GfxMode", "Maximized", display->isMaximized());
    set_config_int("GfxMode", "Width", display->originalWidth());
    set_config_int("GfxMode", "Height", display->originalHeight());

    std::string windowLayout = display->getLayout();
    if (!windowLayout.empty())
      set_config_string("GfxMode", "WindowLayout", windowLayout.c_str());
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

void load_window_pos(Widget* window, const char* section,
                     const bool limitMinSize)
{
  // Default position
  Rect orig_pos = window->bounds();
  Rect pos = orig_pos;

  // Load configurated position
  pos = get_config_rect(section, "WindowPos", pos);

  if (limitMinSize) {
    pos.w = base::clamp(pos.w, orig_pos.w, ui::display_w());
    pos.h = base::clamp(pos.h, orig_pos.h, ui::display_h());
  }
  else {
    pos.w = std::min(pos.w, ui::display_w());
    pos.h = std::min(pos.h, ui::display_h());
  }

  pos.setOrigin(Point(base::clamp(pos.x, 0, ui::display_w()-pos.w),
                      base::clamp(pos.y, 0, ui::display_h()-pos.h)));

  window->setBounds(pos);
}

void save_window_pos(Widget* window, const char *section)
{
  set_config_rect(section, "WindowPos", window->bounds());
}

// TODO Replace this with new theme styles
Widget* setup_mini_font(Widget* widget)
{
  SkinPropertyPtr skinProp = get_skin_property(widget);
  skinProp->setMiniFont();
  return widget;
}

// TODO Replace this with new theme styles
Widget* setup_mini_look(Widget* widget)
{
  SkinPropertyPtr skinProp = get_skin_property(widget);
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

// Manager event handler.
bool CustomizedGuiManager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseDisplayMessage:
      {
        // Execute the "Exit" command.
        Command* command = Commands::instance()->byId(CommandId::Exit());
        UIContext::instance()->executeCommand(command);
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
              ctx->executeCommand(cmd, params);
            }
            // Other extensions will be handled as an image/sprite
            else {
              OpenFileCommand cmd;
              Params params;
              params.set("filename", fn.c_str());
              params.set("repeat_checkbox", "true");
              ctx->executeCommand(&cmd, params);

              // Remove all used file names from the "dropped files"
              for (const auto& usedFn : cmd.usedFiles()) {
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

      KeyboardShortcuts* keys = KeyboardShortcuts::instance();
      for (const KeyPtr& key : *keys) {
        if (key->isPressed(msg, *keys)) {
          // Cancel menu-bar loops (to close any popup menu)
          App::instance()->mainWindow()->getMenuBar()->cancelMenuLoop();

          switch (key->type()) {

            case KeyType::Tool: {
              tools::Tool* current_tool = App::instance()->activeTool();
              tools::Tool* select_this_tool = key->tool();
              tools::ToolBox* toolbox = App::instance()->toolBox();
              std::vector<tools::Tool*> possibles;

              // Collect all tools with the pressed keyboard-shortcut
              for (tools::Tool* tool : *toolbox) {
                const KeyPtr key = KeyboardShortcuts::instance()->tool(tool);
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
              if (getForegroundWindow() == App::instance()->mainWindow()) {
                // OK, so we can execute the command represented
                // by the pressed-key in the message...
                UIContext::instance()->executeCommand(
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

  // F1 switches screen/UI scaling
  if (msg->ctrlPressed() &&
      msg->scancode() == kKeyF1) {
    try {
      os::Display* display = getDisplay();
      int screenScale = display->scale();
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
      if (screenScale != display->scale()) {
        display->setScale(screenScale);
        setDisplay(display);
      }
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
    return true;
  }

#ifdef ENABLE_DATA_RECOVERY
  // Ctrl+Shift+R recover active sprite from the backup store
  if (msg->ctrlPressed() &&
      msg->shiftPressed() &&
      msg->scancode() == kKeyR &&
      App::instance()->dataRecovery() &&
      App::instance()->dataRecovery()->activeSession() &&
      current_editor &&
      current_editor->document()) {
    App::instance()
      ->dataRecovery()
      ->activeSession()
      ->restoreBackupById(current_editor->document()->id());
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

void CustomizedGuiManager::onNewDisplayConfiguration()
{
  Manager::onNewDisplayConfiguration();
  save_gui_config();
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
