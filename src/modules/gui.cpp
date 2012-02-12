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

#include <algorithm>
#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <list>
#include <vector>

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "app.h"
#include "base/memory.h"
#include "base/shared_ptr.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "document_wrappers.h"
#include "drop_files.h"
#include "gfxmode.h"
#include "gui/gui.h"
#include "gui/intern.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/rootmenu.h"
#include "raster/sprite.h"
#include "resource_finder.h"
#include "skin/button_icon_impl.h"
#include "skin/skin_property.h"
#include "skin/skin_theme.h"
#include "tools/ink.h"
#include "tools/tool_box.h"
#include "ui_context.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"
#include "xml_widgets.h"

#define REFRESH_FULL_SCREEN     1
#define SYSTEM_WINDOW_RESIZE    2

#define MONITOR_TIMER_MSECS     100

#define SPRITEDITOR_ACTION_COPYSELECTION        "CopySelection"
#define SPRITEDITOR_ACTION_SNAPTOGRID           "SnapToGrid"
#define SPRITEDITOR_ACTION_ANGLESNAP            "AngleSnap"
#define SPRITEDITOR_ACTION_MAINTAINASPECTRATIO  "MaintainAspectRatio"

//////////////////////////////////////////////////////////////////////

using namespace gfx;

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

struct Monitor
{
  // returns true when the job is done and the monitor can be removed
  void (*proc)(void *);
  void (*free)(void *);
  void *data;
  bool lock;
  bool deleted;

  Monitor(void (*proc)(void *),
          void (*free)(void *), void *data);
  ~Monitor();
};

//////////////////////////////////////////////////////////////////////

static JWidget manager = NULL;
static Theme* ase_theme = NULL;

static int monitor_timer = -1;
static MonitorList* monitors = NULL;
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

static bool button_with_icon_msg_proc(JWidget widget, Message* msg);
static bool manager_msg_proc(JWidget widget, Message* msg);

static void on_palette_change_signal();

// Used by set_display_switch_callback(SWITCH_IN, ...).
static void display_switch_in_callback()
{
  next_idle_flags |= REFRESH_FULL_SCREEN;
}

END_OF_STATIC_FUNCTION(display_switch_in_callback);

// Called when the window is resized
static void resize_callback(RESIZE_DISPLAY_EVENT *ev)
{
   if (ev->is_maximized) {
      restored_width = ev->old_w;
      restored_height = ev->old_h;
   }
   next_idle_flags |= SYSTEM_WINDOW_RESIZE;
}

// Initializes GUI.
int init_module_gui()
{
  int min_possible_dsk_res = 0;
  int c, w, h, bpp, autodetect;
  bool fullscreen;
  bool maximized;

  monitors = new MonitorList;
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
  manager = jmanager_new();
  jwidget_add_hook(manager, JI_WIDGET, manager_msg_proc, NULL);

  // Setup the GUI theme for all widgets
  CurrentTheme::set(ase_theme = new SkinTheme());

  // Setup the handler for window-resize events
  set_resize_callback(resize_callback);

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

  // destroy monitors
  ASSERT(monitors != NULL);
  for (MonitorList::iterator
         it2 = monitors->begin(); it2 != monitors->end(); ++it2) {
    Monitor* monitor = *it2;
    delete monitor;
  }
  delete monitors;
  monitors = NULL;

  if (double_buffering) {
    BITMAP *old_bmp = ji_screen;
    ji_set_screen(screen, SCREEN_W, SCREEN_H);

    if (ji_screen_created)
      destroy_bitmap(old_bmp);
    ji_screen_created = false;
  }

  jmanager_free(manager);

  // Now we can destroy theme
  CurrentTheme::set(NULL);
  delete ase_theme;

  remove_keyboard();
  remove_mouse();
}

Monitor::Monitor(void (*proc)(void *),
                 void (*free)(void *), void *data)
{
  this->proc = proc;
  this->free = free;
  this->data = data;
  this->lock = false;
  this->deleted = false;
}

Monitor::~Monitor()
{
  if (this->free)
    (*this->free)(this->data);
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
      jmanager_refresh_screen();
    }
  }
  // With a document.
  else {
    const Sprite* sprite = document->getSprite();

    // Select the palette of the sprite.
    if (set_current_palette(sprite->getPalette(sprite->getCurrentFrame()), false)) {
      // If the palette changes, refresh the whole screen.
      jmanager_refresh_screen();
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
  jmanager_run(manager);
}

void gui_feedback()
{
  if (next_idle_flags & SYSTEM_WINDOW_RESIZE) {
    next_idle_flags ^= SYSTEM_WINDOW_RESIZE;

    if (acknowledge_resize() < 0)
      set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0);

    gui_setup_screen(false);
    app_get_top_window()->remap_window();
    jmanager_refresh_screen();
  }

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

void load_window_pos(JWidget window, const char *section)
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

void save_window_pos(JWidget window, const char *section)
{
  set_config_rect(section, "WindowPos", window->getBounds());
}

Widget* load_widget(const char* filename, const char* name)
{
  Widget* widget;
  char buf[512];
  bool found = false;

  ResourceFinder rf;

  rf.addPath(filename);

  usprintf(buf, "widgets/%s", filename);
  rf.findInDataDir(buf);

  while (const char* path = rf.next()) {
    if (exists(path)) {
      ustrcpy(buf, path);
      found = true;
      break;
    }
  }

  if (!found)
    throw widget_file_not_found(filename);

  widget = load_widget_from_xmlfile(buf, name);
  if (!widget)
    throw widget_not_found(name);

  return widget;
}

JWidget find_widget(JWidget widget, const char *name)
{
  Widget* child = widget->findChild(name);
  if (!child)
    throw widget_not_found(name);

  return child;
}

//////////////////////////////////////////////////////////////////////
// Hook signals

typedef struct HookData {
  int signal_num;
  bool (*signal_handler)(JWidget widget, void *data);
  void *data;
} HookData;

static int hook_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool hook_msg_proc(JWidget widget, Message* msg)
{
  switch (msg->type) {

    case JM_DESTROY: {
      HookData* hook_data = reinterpret_cast<HookData*>(jwidget_get_data(widget, hook_type()));
      delete hook_data;
      break;
    }

    case JM_SIGNAL: {
      HookData* hook_data = reinterpret_cast<HookData*>(jwidget_get_data(widget, hook_type()));
      if (hook_data &&
          hook_data->signal_num == msg->signal.num) {
        return (*hook_data->signal_handler)(widget, hook_data->data);
      }
      break;
    }
  }

  return false;
}

// @warning You can't use this function for the same widget two times.
void hook_signal(JWidget widget,
                 int signal_num,
                 bool (*signal_handler)(JWidget widget, void* data),
                 void* data)
{
  ASSERT(widget != NULL);
  ASSERT(jwidget_get_data(widget, hook_type()) == NULL);

  HookData* hook_data = new HookData;

  hook_data->signal_num = signal_num;
  hook_data->signal_handler = signal_handler;
  hook_data->data = data;

  jwidget_add_hook(widget, hook_type(), hook_msg_proc, hook_data);
}

/**
 * Utility routine to get various widgets by name.
 *
 * @code
 * if (!get_widgets(wnd,
 *                  "name1", &widget1,
 *                  "name2", &widget2,
 *                  "name3", &widget3,
 *                  NULL)) {
 *   ...
 * }
 * @endcode
 */
void get_widgets(JWidget window, ...)
{
  JWidget *widget;
  char *name;
  va_list ap;

  va_start(ap, window);
  while ((name = va_arg(ap, char *))) {
    widget = va_arg(ap, JWidget *);
    if (!widget)
      break;

    *widget = window->findChild(name);
    if (!*widget)
      throw widget_not_found(name);
  }
  va_end(ap);
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

RadioButton* radio_button_new(int radio_group, int b1, int b2, int b3, int b4)
{
  RadioButton* widget = new RadioButton(NULL, radio_group, JI_BUTTON);

  widget->setRadioGroup(radio_group);
  widget->setAlign(JI_CENTER | JI_MIDDLE);

  setup_mini_look(widget);
  setup_bevels(widget, b1, b2, b3, b4);

  return widget;
}

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

Command* get_command_from_key_message(Message* msg)
{
  for (std::vector<Shortcut*>::iterator
         it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_ExecuteCommand &&
        // TODO why?
        // shortcut->argument.empty() &&
        shortcut->is_pressed(msg)) {
      return shortcut->command;
    }
  }
  return NULL;
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

// Adds a routine to be called each 100 milliseconds to monitor
// whatever you want. It's mainly used to monitor the progress of a
// file-operation (see @ref fop_operate)
Monitor* add_gui_monitor(void (*proc)(void *),
                         void (*free)(void *), void *data)
{
  Monitor* monitor = new Monitor(proc, free, data);

  monitors->push_back(monitor);

  if (monitor_timer < 0)
    monitor_timer = jmanager_add_timer(manager, MONITOR_TIMER_MSECS);

  jmanager_start_timer(monitor_timer);

  return monitor;
}

// Removes and frees a previously added monitor.
void remove_gui_monitor(Monitor* monitor)
{
  MonitorList::iterator it =
    std::find(monitors->begin(), monitors->end(), monitor);

  ASSERT(it != monitors->end());

  if (!monitor->lock)
    delete monitor;
  else
    monitor->deleted = true;

  monitors->erase(it);
  if (monitors->empty())
    jmanager_stop_timer(monitor_timer);
}

void* get_monitor_data(Monitor* monitor)
{
  return monitor->data;
}

// Manager event handler.
static bool manager_msg_proc(JWidget widget, Message* msg)
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

    case JM_TIMER:
      if (msg->timer.timer_id == monitor_timer) {
        for (MonitorList::iterator
               it = monitors->begin(), next; it != monitors->end(); it = next) {
          Monitor* monitor = *it;
          next = it;
          ++next;

          // is the monitor not lock?
          if (!monitor->lock) {
            // call the monitor procedure
            monitor->lock = true;
            (*monitor->proc)(monitor->data);
            monitor->lock = false;

            if (monitor->deleted)
              delete monitor;
          }
        }

        // is monitors empty? we can stop the timer so
        if (monitors->empty())
          jmanager_stop_timer(monitor_timer);
      }
      break;

    case JM_KEYPRESSED: {
      Frame* toplevel_frame = dynamic_cast<Frame*>(jmanager_get_top_window());

      // If there is a foreground window as top level...
      if (toplevel_frame &&
          toplevel_frame != app_get_top_window() &&
          toplevel_frame->is_foreground()) {
        // We just do not process keyboard shortcuts for menus and tools
        break;
      }

      for (std::vector<Shortcut*>::iterator
             it = shortcuts->begin(); it != shortcuts->end(); ++it) {
        Shortcut* shortcut = *it;

        if (shortcut->is_pressed(msg)) {
          // Cancel menu-bar loops (to close any popup menu)
          app_get_menubar()->cancelMenuLoop();

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
                      toolbar_is_tool_visible(app_get_toolbar(), possibles[i])) {
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

              toolbar_select_tool(app_get_toolbar(), select_this_tool);
              return true;
            }

            case Shortcut_ExecuteCommand: {
              Command* command = shortcut->command;

              // Commands are executed only when the main window is
              // the current window running at foreground.
              JLink link;
              JI_LIST_FOR_EACH(widget->children, link) {
                Frame* child = reinterpret_cast<Frame*>(link->data);

                // There are a foreground window executing?
                if (child->is_foreground()) {
                  break;
                }
                // Is it the desktop and the top-window=
                else if (child->is_desktop() && child == app_get_top_window()) {
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

  return false;
}

// Slot for App::PaletteChange to regenerate graphics when the App palette is changed
static void on_palette_change_signal()
{
  // Regenerate the theme
  CurrentTheme::get()->regenerate();
}
