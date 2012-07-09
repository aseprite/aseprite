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
#include "base/memory.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "document_wrappers.h"
#include "drop_files.h"
#include "gfxmode.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/sprite.h"
#include "skin/button_icon_impl.h"
#include "skin/skin_property.h"
#include "skin/skin_theme.h"
#include "tools/ink.h"
#include "tools/tool_box.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "ui_context.h"
#include "widgets/editor/editor.h"
#include "widgets/main_menu_bar.h"
#include "widgets/main_menu_bar.h"
#include "widgets/main_window.h"
#include "widgets/status_bar.h"
#include "widgets/toolbar.h"

#include <algorithm>
#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <list>
#include <vector>

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#define REFRESH_FULL_SCREEN     1
#define SYSTEM_WINDOW_RESIZE    2

#define SPRITEDITOR_ACTION_COPYSELECTION        "CopySelection"
#define SPRITEDITOR_ACTION_SNAPTOGRID           "SnapToGrid"
#define SPRITEDITOR_ACTION_ANGLESNAP            "AngleSnap"
#define SPRITEDITOR_ACTION_MAINTAINASPECTRATIO  "MaintainAspectRatio"
#define SPRITEDITOR_ACTION_LOCKAXIS             "LockAxis"

//////////////////////////////////////////////////////////////////////

using namespace gfx;
using namespace ui;

static struct
{
  int width;
  int height;
  int scale;
} try_resolutions[] = { { 1024, 768, 2 },
                        {  800, 600, 2 },
                        {  640, 480, 2 },
                        {  320, 240, 1 },
                        {  320, 200, 1 },
                        {    0,   0, 0 } };

static int try_depths[] = { 32, 24, 16, 15 };

static GfxMode lastWorkingGfxMode;

//////////////////////////////////////////////////////////////////////

enum ShortcutType { Shortcut_ExecuteCommand,
                    Shortcut_ChangeTool,
                    Shortcut_EditorQuicktool,
                    Shortcut_SpriteEditor };

struct Shortcut
{
  JAccel accel;
  ShortcutType type;
  union {
    Command* command;
    tools::Tool* tool;
    char* action;
  };
  Params* params;

  Shortcut(ShortcutType type);
  ~Shortcut();

  void add_shortcut(const char* shortcut_string);
  bool is_pressed(Message* msg);
  bool is_pressed_from_key_array();

};

static Shortcut* get_keyboard_shortcut_for_command(const char* command_name, Params* params);
static Shortcut* get_keyboard_shortcut_for_tool(tools::Tool* tool);
static Shortcut* get_keyboard_shortcut_for_quicktool(tools::Tool* tool);
static Shortcut* get_keyboard_shortcut_for_spriteeditor(const char* action_name);

//////////////////////////////////////////////////////////////////////

class CustomizedGuiManager : public Manager
{
protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
};

static CustomizedGuiManager* manager = NULL;
static Theme* ase_theme = NULL;

static std::vector<Shortcut*>* shortcuts = NULL;

static bool ji_screen_created = false;

static volatile int next_idle_flags = 0;

static volatile int restored_width = 0;
static volatile int restored_height = 0;

// Default GUI screen configuration
static bool double_buffering;
static int screen_scaling;

static void reload_default_font();

// Load & save graphics configuration
static void load_gui_config(int& w, int& h, int& bpp, bool& fullscreen, bool& maximized);
static void save_gui_config();

static bool button_with_icon_msg_proc(Widget* widget, Message* msg);

static void on_palette_change_signal();

// Used by set_display_switch_callback(SWITCH_IN, ...).
static void display_switch_in_callback()
{
  next_idle_flags |= REFRESH_FULL_SCREEN;
}

END_OF_STATIC_FUNCTION(display_switch_in_callback);

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
// Called when the window is resized
static void resize_callback(RESIZE_DISPLAY_EVENT *ev)
{
   if (ev->is_maximized) {
      restored_width = ev->old_w;
      restored_height = ev->old_h;
   }
   next_idle_flags |= SYSTEM_WINDOW_RESIZE;
}
#endif // ALLEGRO4_WITH_RESIZE_PATCH

// Initializes GUI.
int init_module_gui()
{
  int min_possible_dsk_res = 0;
  int c, w, h, bpp, autodetect;
  bool fullscreen;
  bool maximized;

  shortcuts = new std::vector<Shortcut*>;

  // Install the mouse
  if (install_mouse() < 0)
    throw base::Exception("Error installing mouse handler");

  // Install the keyboard
  if (install_keyboard() < 0)
    throw base::Exception("Error installing keyboard handler");

  // Disable Ctrl+Shift+End in non-DOS
#if !defined(ALLEGRO_DOS)
  three_finger_flag = false;
#endif
  three_finger_flag = true;     // TODO remove this line

  // Set the graphics mode...
  load_gui_config(w, h, bpp, fullscreen, maximized);

  autodetect = fullscreen ? GFX_AUTODETECT_FULLSCREEN:
                            GFX_AUTODETECT_WINDOWED;

  // Default resolution
  if (!w || !h) {
    bool has_desktop = false;
    int dsk_w, dsk_h;

    has_desktop = get_desktop_resolution(&dsk_w, &dsk_h) == 0;

#ifndef FULLSCREEN_PLATFORM
    // We must extract some space for the windows borders
    dsk_w -= 16;
    dsk_h -= 32;
#endif

    // Try to get desktop resolution
    if (has_desktop) {
      for (c=0; try_resolutions[c].width; ++c) {
        if (try_resolutions[c].width <= dsk_w &&
            try_resolutions[c].height <= dsk_h) {
          min_possible_dsk_res = c;
          fullscreen = false;
          w = try_resolutions[c].width;
          h = try_resolutions[c].height;
          screen_scaling = try_resolutions[c].scale;
          break;
        }
      }
    }
    // Full screen
    else {
      fullscreen = true;
      w = 320;
      h = 200;
      screen_scaling = 1;
    }
  }

  // Default color depth
  if (!bpp) {
    bpp = desktop_color_depth();
    if (!bpp)
      bpp = 16;
  }

  for (;;) {
    if (bpp == 8)
      throw base::Exception("You cannot use ASEPRITE in 8 bits per pixel");

    // Original
    set_color_depth(bpp);
    if (set_gfx_mode(autodetect, w, h, 0, 0) == 0)
      break;

    for (c=min_possible_dsk_res; try_resolutions[c].width; ++c) {
      if (set_gfx_mode(autodetect,
                       try_resolutions[c].width,
                       try_resolutions[c].height, 0, 0) == 0) {
        screen_scaling = try_resolutions[c].scale;
        goto gfx_done;
      }
    }

    if (bpp == 15)
      throw base::Exception("Error setting graphics mode\n%s\n"
                            "Try \"ase -res WIDTHxHEIGHTxBPP\"\n", allegro_error);

    for (c=0; try_depths[c]; ++c) {
      if (bpp == try_depths[c]) {
        bpp = try_depths[c+1];
        break;
      }
    }
  }

gfx_done:;

  // Create the default-manager
  manager = new CustomizedGuiManager();

  // Setup the GUI theme for all widgets
  CurrentTheme::set(ase_theme = new SkinTheme());

#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  // Setup the handler for window-resize events
  set_resize_callback(resize_callback);
#endif

  #ifdef ALLEGRO_WINDOWS
  if (maximized) {
    ShowWindow(win_get_window(), SW_MAXIMIZE);
  }
  #endif

  // Configure ji_screen
  gui_setup_screen(true);

  // Add a hook to display-switch so when the user returns to the
  // screen it's completelly refreshed/redrawn.
  LOCK_VARIABLE(next_idle_flags);
  LOCK_FUNCTION(display_switch_in_callback);
  set_display_switch_callback(SWITCH_IN, display_switch_in_callback);

  // Set graphics options for next time
  save_gui_config();

  // Hook for palette change to regenerate the theme
  App::instance()->PaletteChange.connect(&on_palette_change_signal);

  return 0;
}

void exit_module_gui()
{
  // destroy shortcuts
  ASSERT(shortcuts != NULL);
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;
    delete shortcut;
  }
  delete shortcuts;
  shortcuts = NULL;

  if (double_buffering) {
    BITMAP *old_bmp = ji_screen;
    ji_set_screen(screen, SCREEN_W, SCREEN_H);

    if (ji_screen_created)
      destroy_bitmap(old_bmp);
    ji_screen_created = false;
  }

  delete manager;

  // Now we can destroy theme
  CurrentTheme::set(NULL);
  delete ase_theme;

  remove_keyboard();
  remove_mouse();
}

static void load_gui_config(int& w, int& h, int& bpp, bool& fullscreen, bool& maximized)
{
  w = get_config_int("GfxMode", "Width", 0);
  h = get_config_int("GfxMode", "Height", 0);
  bpp = get_config_int("GfxMode", "Depth", 0);
  fullscreen = get_config_bool("GfxMode", "FullScreen",
#ifdef FULLSCREEN_PLATFORM
                               true
#else
                               false
#endif
                               );
  screen_scaling = get_config_int("GfxMode", "ScreenScale", 2);
  screen_scaling = MID(1, screen_scaling, 4);
  maximized = get_config_bool("GfxMode", "Maximized", false);

  // Avoid 8 bpp
  if (bpp == 8)
    bpp = 32;
}

static void save_gui_config()
{
  bool is_maximized = false;

#ifdef WIN32
  is_maximized = (GetWindowLong(win_get_window(), GWL_STYLE) & WS_MAXIMIZE ? true: false);
#endif

  set_config_bool("GfxMode", "Maximized", is_maximized);

  if (screen) {
    set_config_int("GfxMode", "Width", is_maximized ? restored_width: SCREEN_W);
    set_config_int("GfxMode", "Height", is_maximized ? restored_height: SCREEN_H);
    set_config_int("GfxMode", "Depth", bitmap_color_depth(screen));
  }

  if (gfx_driver)
    set_config_bool("GfxMode", "FullScreen", gfx_driver->windowed ? false: true);

  set_config_int("GfxMode", "ScreenScale", screen_scaling);
}

int get_screen_scaling()
{
  return screen_scaling;
}

void set_screen_scaling(int scaling)
{
  screen_scaling = scaling;
}

void update_screen_for_document(const Document* document)
{
  // Without document.
  if (!document) {
    // Well, change to the default palette.
    if (set_current_palette(NULL, false)) {
      // If the palette changes, refresh the whole screen.
      Manager::getDefault()->invalidate();
    }
  }
  // With a document.
  else {
    const Sprite* sprite = document->getSprite();

    // Select the palette of the sprite.
    if (set_current_palette(sprite->getPalette(sprite->getCurrentFrame()), false)) {
      // If the palette changes, invalidate the whole screen, we've to
      // redraw it.
      Manager::getDefault()->invalidate();
    }
    else {
      // If it's the same palette update only the editors with the sprite.
      update_editors_with_document(document);
    }

    // Update the tabs (maybe the modified status has been changed).
    app_update_document_tab(document);
  }
}

void gui_run()
{
  manager->run();
}

void gui_feedback()
{
#ifdef ALLEGRO4_WITH_RESIZE_PATCH
  if (next_idle_flags & SYSTEM_WINDOW_RESIZE) {
    next_idle_flags ^= SYSTEM_WINDOW_RESIZE;

    if (acknowledge_resize() < 0)
      set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0);

    gui_setup_screen(false);
    App::instance()->getMainWindow()->remap_window();
    Manager::getDefault()->invalidate();
  }
#endif

  if (next_idle_flags & REFRESH_FULL_SCREEN) {
    next_idle_flags ^= REFRESH_FULL_SCREEN;

    try {
      const ActiveDocumentReader document(UIContext::instance());
      update_screen_for_document(document);
    }
    catch (...) {
      // do nothing
    }
  }

  gui_flip_screen();
}

void gui_flip_screen()
{
  // Double buffering?
  if (double_buffering && ji_screen && screen) {
    jmouse_draw_cursor();

    if (ji_dirty_region) {
      ji_flip_dirty_region();
    }
    else {
      if (JI_SCREEN_W == SCREEN_W && JI_SCREEN_H == SCREEN_H) {
        blit(ji_screen, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      }
      else {
        stretch_blit(ji_screen, screen,
                     0, 0, JI_SCREEN_W, JI_SCREEN_H,
                     0, 0, SCREEN_W, SCREEN_H);
      }
    }
  }
  // This is a strange Allegro bug where the "screen" pointer is lost,
  // so we have to reconstruct it changing the gfx mode again
  else if (screen == NULL) {
    PRINTF("Gfx mode lost, trying to restore gfx mode...\n");

    if (!lastWorkingGfxMode.setGfxMode()) {
      PRINTF("Fatal error\n");
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("FATAL ERROR: Unable to restore the old graphics mode!\n");
      exit(1);
    }

    PRINTF("Successfully restored\n");
  }
  else
    lastWorkingGfxMode.updateWithCurrentMode();
}

// Sets the ji_screen variable. This routine should be called
// everytime you changes the graphics mode.
void gui_setup_screen(bool reload_font)
{
  bool regen = false;
  bool reinit = false;

  // Double buffering is required when screen scaling is used
  double_buffering = (screen_scaling > 1);

  // Is double buffering active?
  if (double_buffering) {
    BITMAP *old_bmp = ji_screen;
    int new_w = SCREEN_W / screen_scaling;
    int new_h = SCREEN_H / screen_scaling;
    BITMAP *new_bmp = create_bitmap(new_w, new_h);

    ji_set_screen(new_bmp, new_w, new_h);

    if (ji_screen_created)
      destroy_bitmap(old_bmp);

    ji_screen_created = true;
  }
  else {
    ji_set_screen(screen, SCREEN_W, SCREEN_H);
    ji_screen_created = false;
  }

  // Update guiscale factor
  int old_guiscale = jguiscale();
  CurrentTheme::get()->guiscale = (screen_scaling == 1 &&
                                   JI_SCREEN_W > 512 &&
                                   JI_SCREEN_H > 256) ? 2: 1;

  // If the guiscale have changed
  if (old_guiscale != jguiscale()) {
    reload_font = true;
    regen = true;
  }

  if (reload_font) {
    reload_default_font();
    reinit = true;
  }

  // Regenerate the theme
  if (regen) {
    CurrentTheme::get()->regenerate();
    reinit = true;
  }

  if (reinit)
    _ji_reinit_theme_in_all_widgets();

  // Set the configuration
  save_gui_config();
}

static void reload_default_font()
{
  Theme* theme = CurrentTheme::get();
  SkinTheme* skin_theme = static_cast<SkinTheme*>(theme);

  // Reload theme fonts
  skin_theme->reload_fonts();

  // Set all widgets fonts
  _ji_set_font_of_all_widgets(theme->default_font);
}

void load_window_pos(Widget* window, const char *section)
{
  // Default position
  Rect orig_pos = window->getBounds();
  Rect pos = orig_pos;

  // Load configurated position
  pos = get_config_rect(section, "WindowPos", pos);

  pos.w = MID(orig_pos.w, pos.w, JI_SCREEN_W);
  pos.h = MID(orig_pos.h, pos.h, JI_SCREEN_H);

  pos.setOrigin(Point(MID(0, pos.x, JI_SCREEN_W-pos.w),
                      MID(0, pos.y, JI_SCREEN_H-pos.h)));

  window->setBounds(pos);
}

void save_window_pos(Widget* window, const char *section)
{
  set_config_rect(section, "WindowPos", window->getBounds());
}

void setup_mini_look(Widget* widget)
{
  setup_look(widget, MiniLook);
}

void setup_look(Widget* widget, LookType lookType)
{
  SharedPtr<SkinProperty> skinProp;

  skinProp = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinProp == NULL)
    skinProp.reset(new SkinProperty);

  skinProp->setLook(lookType);
  widget->setProperty(skinProp);
}

void setup_bevels(Widget* widget, int b1, int b2, int b3, int b4)
{
  SharedPtr<SkinProperty> skinProp;

  skinProp = widget->getProperty(SkinProperty::SkinPropertyName);
  if (skinProp == NULL)
    skinProp.reset(new SkinProperty);

  skinProp->setUpperLeft(b1);
  skinProp->setUpperRight(b2);
  skinProp->setLowerLeft(b3);
  skinProp->setLowerRight(b4);

  widget->setProperty(skinProp);
}

// Sets the IconInterface pointer interface of the button to show the
// specified set of icons. Each icon is a part of the SkinTheme.
void set_gfxicon_to_button(ButtonBase* button,
                           int normal_part_id,
                           int selected_part_id,
                           int disabled_part_id, int icon_align)
{
  ButtonIconImpl* buttonIcon =
    new ButtonIconImpl(static_cast<SkinTheme*>(button->getTheme()),
                       normal_part_id,
                       selected_part_id,
                       disabled_part_id,
                       icon_align);

  button->setIconInterface(buttonIcon);
}

//////////////////////////////////////////////////////////////////////
// Button style (convert radio or check buttons and draw it like
// normal buttons)

CheckBox* check_button_new(const char *text, int b1, int b2, int b3, int b4)
{
  CheckBox* widget = new CheckBox(text, JI_BUTTON);

  widget->setAlign(JI_CENTER | JI_MIDDLE);

  setup_mini_look(widget);
  setup_bevels(widget, b1, b2, b3, b4);
  return widget;
}

//////////////////////////////////////////////////////////////////////
// Keyboard shortcuts
//////////////////////////////////////////////////////////////////////

JAccel add_keyboard_shortcut_to_execute_command(const char* shortcut_string, const char* command_name, Params* params)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_command(command_name, params);
  if (!shortcut) {
    shortcut = new Shortcut(Shortcut_ExecuteCommand);
    shortcut->command = CommandsModule::instance()->getCommandByName(command_name);
    shortcut->params = params ? params->clone(): new Params;

    shortcuts->push_back(shortcut);
  }

  shortcut->add_shortcut(shortcut_string);
  return shortcut->accel;
}

JAccel add_keyboard_shortcut_to_change_tool(const char* shortcut_string, tools::Tool* tool)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_tool(tool);
  if (!shortcut) {
    shortcut = new Shortcut(Shortcut_ChangeTool);
    shortcut->tool = tool;

    shortcuts->push_back(shortcut);
  }

  shortcut->add_shortcut(shortcut_string);
  return shortcut->accel;
}

JAccel add_keyboard_shortcut_to_quicktool(const char* shortcut_string, tools::Tool* tool)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_quicktool(tool);
  if (!shortcut) {
    shortcut = new Shortcut(Shortcut_EditorQuicktool);
    shortcut->tool = tool;

    shortcuts->push_back(shortcut);
  }

  shortcut->add_shortcut(shortcut_string);
  return shortcut->accel;
}

JAccel add_keyboard_shortcut_to_spriteeditor(const char* shortcut_string, const char* action_name)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(action_name);
  if (!shortcut) {
    shortcut = new Shortcut(Shortcut_SpriteEditor);
    shortcut->action = base_strdup(action_name);

    shortcuts->push_back(shortcut);
  }

  shortcut->add_shortcut(shortcut_string);
  return shortcut->accel;
}

bool get_command_from_key_message(Message* msg, Command** command, Params** params)
{
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_ExecuteCommand && shortcut->is_pressed(msg)) {
      if (command) *command = shortcut->command;
      if (params) *params = shortcut->params;
      return true;
    }
  }
  return false;
}

JAccel get_accel_to_execute_command(const char* command_name, Params* params)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_command(command_name, params);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_change_tool(tools::Tool* tool)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_tool(tool);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_copy_selection()
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(SPRITEDITOR_ACTION_COPYSELECTION);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_snap_to_grid()
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(SPRITEDITOR_ACTION_SNAPTOGRID);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_angle_snap()
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(SPRITEDITOR_ACTION_ANGLESNAP);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_maintain_aspect_ratio()
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(SPRITEDITOR_ACTION_MAINTAINASPECTRATIO);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

JAccel get_accel_to_lock_axis()
{
  Shortcut* shortcut = get_keyboard_shortcut_for_spriteeditor(SPRITEDITOR_ACTION_LOCKAXIS);
  if (shortcut)
    return shortcut->accel;
  else
    return NULL;
}

tools::Tool* get_selected_quicktool(tools::Tool* currentTool)
{
  if (currentTool && currentTool->getInk(0)->isSelection()) {
    JAccel copyselection_accel = get_accel_to_copy_selection();
    if (copyselection_accel && jaccel_check_from_key(copyselection_accel))
      return NULL;
  }

  tools::ToolBox* toolbox = App::instance()->getToolBox();

  // Iterate over all tools
  for (tools::ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Shortcut* shortcut = get_keyboard_shortcut_for_quicktool(*it);

    // Collect all tools with the pressed keyboard-shortcut
    if (shortcut && shortcut->is_pressed_from_key_array()) {
      return *it;
    }
  }

  return NULL;
}

Shortcut::Shortcut(ShortcutType type)
{
  this->type = type;
  this->accel = jaccel_new();
  this->command = NULL;
  this->tool = NULL;
  this->params = NULL;
}

Shortcut::~Shortcut()
{
  delete params;
  if (type == Shortcut_SpriteEditor)
    base_free(action);
  jaccel_free(accel);
}

void Shortcut::add_shortcut(const char* shortcut_string)
{
  char buf[256];
  usprintf(buf, "<%s>", shortcut_string);
  jaccel_add_keys_from_string(this->accel, buf);
}

bool Shortcut::is_pressed(Message* msg)
{
  if (accel) {
    return jaccel_check(accel,
                        msg->any.shifts,
                        msg->key.ascii,
                        msg->key.scancode);
  }
  return false;
}

bool Shortcut::is_pressed_from_key_array()
{
  if (accel) {
    return jaccel_check_from_key(accel);
  }
  return false;
}

static Shortcut* get_keyboard_shortcut_for_command(const char* command_name, Params* params)
{
  Command* command = CommandsModule::instance()->getCommandByName(command_name);
  if (!command)
    return NULL;

  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_ExecuteCommand &&
        shortcut->command == command &&
        ((!params && shortcut->params->empty()) ||
         (params && *shortcut->params == *params))) {
      return shortcut;
    }
  }

  return NULL;
}

static Shortcut* get_keyboard_shortcut_for_tool(tools::Tool* tool)
{
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_ChangeTool &&
        shortcut->tool == tool) {
      return shortcut;
    }
  }

  return NULL;
}

static Shortcut* get_keyboard_shortcut_for_quicktool(tools::Tool* tool)
{
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_EditorQuicktool &&
        shortcut->tool == tool) {
      return shortcut;
    }
  }

  return NULL;
}

static Shortcut* get_keyboard_shortcut_for_spriteeditor(const char* action_name)
{
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_SpriteEditor &&
        strcmp(shortcut->action, action_name) == 0) {
      return shortcut;
    }
  }

  return NULL;
}

// Manager event handler.
bool CustomizedGuiManager::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE_APP:
      {
        // Execute the "Exit" command.
        Command* command = CommandsModule::instance()->getCommandByName(CommandId::Exit);
        UIContext::instance()->executeCommand(command);
      }
      break;

    case JM_QUEUEPROCESSING:
      gui_feedback();

      // Open dropped files
      check_for_dropped_files();
      break;

    case JM_KEYPRESSED: {
      Window* toplevel_window = getTopWindow();

      // If there is a foreground window as top level...
      if (toplevel_window &&
          toplevel_window != App::instance()->getMainWindow() &&
          toplevel_window->is_foreground()) {
        // We just do not process keyboard shortcuts for menus and tools
        break;
      }

      for (std::vector<Shortcut*>::iterator
             it = shortcuts->begin(); it != shortcuts->end(); ++it) {
        Shortcut* shortcut = *it;

        if (shortcut->is_pressed(msg)) {
          // Cancel menu-bar loops (to close any popup menu)
          App::instance()->getMainWindow()->getMenuBar()->cancelMenuLoop();

          switch (shortcut->type) {

            case Shortcut_ChangeTool: {
              tools::Tool* current_tool = UIContext::instance()->getSettings()->getCurrentTool();
              tools::Tool* select_this_tool = shortcut->tool;
              tools::ToolBox* toolbox = App::instance()->getToolBox();
              std::vector<tools::Tool*> possibles;

              // Iterate over all tools
              for (tools::ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
                Shortcut* shortcut = get_keyboard_shortcut_for_tool(*it);

                // Collect all tools with the pressed keyboard-shortcut
                if (shortcut && shortcut->is_pressed(msg))
                  possibles.push_back(*it);
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

            case Shortcut_ExecuteCommand: {
              Command* command = shortcut->command;

              // Commands are executed only when the main window is
              // the current window running at foreground.
              JLink link;
              JI_LIST_FOR_EACH(this->children, link) {
                Window* child = reinterpret_cast<Window*>(link->data);

                // There are a foreground window executing?
                if (child->is_foreground()) {
                  break;
                }
                // Is it the desktop and the top-window=
                else if (child->is_desktop() && child == App::instance()->getMainWindow()) {
                  // OK, so we can execute the command represented
                  // by the pressed-key in the message...
                  UIContext::instance()->executeCommand(command, shortcut->params);
                  return true;
                }
              }
              break;
            }

            case Shortcut_EditorQuicktool: {
              // Do nothing, it is used in the editor through the
              // get_selected_quicktool() function.
              break;
            }

          }
          break;
        }
      }
      break;
    }

  }

  return Manager::onProcessMessage(msg);
}

// Slot for App::PaletteChange to regenerate graphics when the App palette is changed
static void on_palette_change_signal()
{
  // Regenerate the theme
  CurrentTheme::get()->regenerate();
}
