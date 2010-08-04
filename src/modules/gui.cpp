/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "Vaca/SharedPtr.h"
#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/drop_files.h"
#include "gfxmode.h"
#include "intl/msgids.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/rootmenu.h"
#include "modules/skinneable_theme.h"
#include "raster/sprite.h"
#include "resource_finder.h"
#include "sprite_wrappers.h"
#include "tools/toolbox.h"
#include "ui_context.h"
#include "util/recscr.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"

#define REBUILD_RECENT_LIST	2
#define REFRESH_FULL_SCREEN	4
#define SYSTEM_WINDOW_RESIZE    8

#define MONITOR_TIMER_MSECS	100

//////////////////////////////////////////////////////////////////////

#ifdef ALLEGRO_WINDOWS
#  define DEF_SCALE 2
#else
#  define DEF_SCALE 1
#endif

static struct
{
  int width;
  int height;
  int scale;
} try_resolutions[] = { { 1024, 768, DEF_SCALE },
			{  800, 600, DEF_SCALE },
			{  640, 480, DEF_SCALE },
			{  320, 240, 1 },
			{  320, 200, 1 },
			{    0,   0, 0 } };

static int try_depths[] = { 32, 24, 16, 15 };

static GfxMode lastWorkingGfxMode;

//////////////////////////////////////////////////////////////////////

enum ShortcutType { Shortcut_ExecuteCommand,
		    Shortcut_ChangeTool };

struct Shortcut
{
  JAccel accel;
  ShortcutType type;
  union {
    Command* command;
    Tool* tool;
  };
  Params* params;

  Shortcut(ShortcutType type);
  ~Shortcut();

  void add_shortcut(const char* shortcut_string);
  bool is_key_pressed(JMessage msg);

};

static Shortcut* get_keyboard_shortcut_for_command(const char* command_name, Params* params);
static Shortcut* get_keyboard_shortcut_for_tool(Tool* tool);

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
static JTheme ase_theme = NULL;

static int monitor_timer = -1;
static MonitorList* monitors = NULL;
static std::vector<Shortcut*>* shortcuts = NULL;

static bool ji_screen_created = false;

static volatile int next_idle_flags = 0;
static JList icon_buttons;

/* default GUI screen configuration */
static bool double_buffering;
static int screen_scaling;

static void reload_default_font();

/* load & save graphics configuration */
static void load_gui_config(int& w, int& h, int& bpp, bool& fullscreen, bool& maximized);
static void save_gui_config();

static bool button_with_icon_msg_proc(JWidget widget, JMessage msg);
static bool manager_msg_proc(JWidget widget, JMessage msg);

static void on_palette_change_signal();

/**
 * Used by set_display_switch_callback(SWITCH_IN, ...).
 */
static void display_switch_in_callback()
{
  next_idle_flags |= REFRESH_FULL_SCREEN;
}

END_OF_STATIC_FUNCTION(display_switch_in_callback);

#ifdef HAVE_RESIZE_PATCH
static void resize_callback(void)
{
  next_idle_flags |= SYSTEM_WINDOW_RESIZE;
}
#endif

/**
 * Initializes GUI.
 */
int init_module_gui()
{
  int min_possible_dsk_res = 0;
  int c, w, h, bpp, autodetect;
  bool fullscreen;
  bool maximized;

  monitors = new MonitorList;
  shortcuts = new std::vector<Shortcut*>;

  /* install the mouse */
  if (install_mouse() < 0) {
    user_printf(_("Error installing mouse handler\n"));
    return -1;
  }

  /* install the keyboard */
  if (install_keyboard() < 0) {
    user_printf(_("Error installing keyboard handler\n"));
    return -1;
  }

  /* disable Ctrl+Shift+End in non-DOS */
#if !defined(ALLEGRO_DOS)
  three_finger_flag = false;
#endif
  three_finger_flag = true;	/* TODO remove this line */

  /* set the graphics mode... */
  load_gui_config(w, h, bpp, fullscreen, maximized);

  autodetect = fullscreen ? GFX_AUTODETECT_FULLSCREEN:
			    GFX_AUTODETECT_WINDOWED;

  /* default resolution */
  if (!w || !h) {
    bool has_desktop = false;
    int dsk_w, dsk_h;

    has_desktop = get_desktop_resolution(&dsk_w, &dsk_h) == 0;

    /* we must extract some space for the windows borders */
    dsk_w -= 16;
    dsk_h -= 32;

    /* try to get desktop resolution */
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
    /* full screen */
    else {
      fullscreen = true;
      w = 320;
      h = 200;
      screen_scaling = 1;
    }
  }

  /* default color depth */
  if (!bpp) {
    bpp = desktop_color_depth();
    if (!bpp)
      bpp = 16;
  }

  for (;;) {
    if (bpp == 8) {
      allegro_message("You cannot use ASE in 8 bits per pixel\n");
      return -1;
    }

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

    if (bpp == 15) {
      user_printf(_("Error setting graphics mode\n%s\n"
		    "Try \"ase -res WIDTHxHEIGHTxBPP\"\n"), allegro_error);
      return -1;
    }
    else {
      for (c=0; try_depths[c]; ++c) {
	if (bpp == try_depths[c]) {
	  bpp = try_depths[c+1];
	  break;
	}
      }
    }
  }
 gfx_done:;

  /* window title */
  set_window_title(PACKAGE " v" VERSION);

  /* create the default-manager */
  manager = jmanager_new();
  jwidget_add_hook(manager, JI_WIDGET, manager_msg_proc, NULL);

  /* setup the standard jinete theme for widgets */
  ji_set_theme(ase_theme = new SkinneableTheme());

  /* set hook to translate strings */
  ji_set_translation_hook(msgids_get);

#ifdef HAVE_RESIZE_PATCH
  set_resize_callback(resize_callback);

  #ifdef ALLEGRO_WINDOWS
  if (maximized) {
    ShowWindow(win_get_window(), SW_MAXIMIZE);
  }
  #endif
#endif

  /* configure ji_screen */
  gui_setup_screen(true);

  /* add a hook to display-switch so when the user returns to the
     screen it's completelly refreshed/redrawn */
  LOCK_VARIABLE(next_idle_flags);
  LOCK_FUNCTION(display_switch_in_callback);
  set_display_switch_callback(SWITCH_IN, display_switch_in_callback);

  /* set graphics options for next time */
  save_gui_config();

  // Hook for palette change to regenerate the theme
  App::instance()->PaletteChange.connect(&on_palette_change_signal);

  /* icon buttons */
  icon_buttons = jlist_new();

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

  jlist_free(icon_buttons);
  icon_buttons = NULL;

  jmanager_free(manager);
  ji_set_theme(NULL);
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
  fullscreen = get_config_bool("GfxMode", "FullScreen", false);
  screen_scaling = get_config_int("GfxMode", "Scale", 1);
  screen_scaling = MID(1, screen_scaling, 4);

#if defined HAVE_RESIZE_PATCH && defined ALLEGRO_WINDOWS
  maximized = get_config_bool("GfxMode", "Maximized", false);
#else
  maximized = false;
#endif

  // Avoid 8 bpp
  if (bpp == 8)
    bpp = 32;
}

static void save_gui_config()
{
  if (screen) {
    set_config_int("GfxMode", "Width", SCREEN_W);
    set_config_int("GfxMode", "Height", SCREEN_H);
    set_config_int("GfxMode", "Depth", bitmap_color_depth(screen));
  }

  if (gfx_driver)
    set_config_bool("GfxMode", "FullScreen", gfx_driver->windowed ? false: true);

  set_config_int("GfxMode", "Scale", screen_scaling);

#if defined HAVE_RESIZE_PATCH && defined ALLEGRO_WINDOWS
  set_config_bool("GfxMode", "Maximized",
		  GetWindowLong(win_get_window(), GWL_STYLE) & WS_MAXIMIZE ? true: false);
#endif
}

int get_screen_scaling()
{
  return screen_scaling;
}

void set_screen_scaling(int scaling)
{
  screen_scaling = scaling;
}

void update_screen_for_sprite(const Sprite* sprite)
{
  if (!(ase_mode & MODE_GUI))
    return;

  /* without sprite */
  if (!sprite) {
    /* well, change to the default palette */
    if (set_current_palette(NULL, false)) {
      /* if the palette changes, refresh the whole screen */
      jmanager_refresh_screen();
    }
  }
  /* with a sprite */
  else {
    /* select the palette of the sprite */
    if (set_current_palette(sprite->getPalette(sprite->getCurrentFrame()), false)) {
      /* if the palette changes, refresh the whole screen */
      jmanager_refresh_screen();
    }
    else {
      /* if it's the same palette update only the editors with the sprite */
      update_editors_with_sprite(sprite);
    }
  }
}

void gui_run()
{
  jmanager_run(manager);
}

void gui_feedback()
{
#ifdef HAVE_RESIZE_PATCH
  if (next_idle_flags & SYSTEM_WINDOW_RESIZE) {
    next_idle_flags ^= SYSTEM_WINDOW_RESIZE;

    resize_screen();
    gui_setup_screen(false);
    app_get_top_window()->remap_window();
    jmanager_refresh_screen();
  }
#endif

  /* menu stuff */
  if (next_idle_flags & REBUILD_RECENT_LIST) {
    if (app_realloc_recent_list())
      next_idle_flags ^= REBUILD_RECENT_LIST;
  }

  if (next_idle_flags & REFRESH_FULL_SCREEN) {
    next_idle_flags ^= REFRESH_FULL_SCREEN;

    try {
      const CurrentSpriteReader sprite(UIContext::instance());
      update_screen_for_sprite(sprite);
    }
    catch (...) {
      // do nothing
    }
  }

  /* record file if is necessary */
  rec_screen_poll();

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
      user_printf(_("FATAL ERROR: Unable to restore the old graphics mode!\n"));
      exit(1);
    }

    PRINTF("Successfully restored\n");
  }
  else
    lastWorkingGfxMode.updateWithCurrentMode();
}

/**
 * Sets the ji_screen variable.
 * 
 * This routine should be called everytime you changes the graphics
 * mode.
 */
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
  ji_get_theme()->guiscale = (screen_scaling == 1 &&
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
    ji_regen_theme();
    reinit = true;
  }

  if (reinit)
    _ji_reinit_theme_in_all_widgets();

  // Set the configuration
  save_gui_config();
}

static void reload_default_font()
{
  JTheme theme = ji_get_theme();
  SkinneableTheme* skinneable_theme = static_cast<SkinneableTheme*>(theme);
  const char *user_font;

  // No font for now
  if (theme->default_font && theme->default_font != font)
    destroy_font(theme->default_font);

  theme->default_font = NULL;

  // Directories
  ResourceFinder rf;

  user_font = get_config_string("Options", "UserFont", "");
  if ((user_font) && (*user_font))
    rf.addPath(user_font);

  // TODO This should be in SkinneableTheme class
  rf.findInDataDir(skinneable_theme->get_font_filename().c_str());

  // Try to load the font
  while (const char* path = rf.next()) {
    theme->default_font = ji_font_load(path);
    if (theme->default_font) {
      if (ji_font_is_scalable(theme->default_font))
	ji_font_set_size(theme->default_font, 8*jguiscale());
      break;
    }
  }

  // default font: the Allegro one
  if (!theme->default_font)
    theme->default_font = font;

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

JWidget load_widget(const char *filename, const char *name)
{
  JWidget widget;
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

  widget = ji_load_widget(buf, name);
  if (!widget)
    throw widget_not_found(name);

  return widget;
}

JWidget find_widget(JWidget widget, const char *name)
{
  JWidget child = jwidget_find_name(widget, name);
  if (!child)
    throw widget_not_found(name);

  return child;
}

void schedule_rebuild_recent_list()
{
  next_idle_flags |= REBUILD_RECENT_LIST;
}

/**********************************************************************/
/* hook signals */

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

static bool hook_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      jfree(jwidget_get_data(widget, hook_type()));
      break;

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

/**
 * @warning You can't use this function for the same widget two times.
 */
void hook_signal(JWidget widget,
		 int signal_num,
		 bool (*signal_handler)(JWidget widget, void* data),
		 void* data)
{
  HookData* hook_data = jnew(HookData, 1);

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

    *widget = jwidget_find_name(window, name);
    if (!*widget)
      throw widget_not_found(name);
  }
  va_end(ap);
}

void setup_mini_look(Widget* widget)
{
  Vaca::SharedPtr<SkinProperty> skinProp(new SkinProperty);
  skinProp->setMiniLook(true);
  widget->setProperty(skinProp);
}

/**********************************************************************/
/* Icon in buttons */

/* adds a button in the list of "icon_buttons" to restore the icon
   when the palette changes, the "user_data[3]" is used to save the
   "gfx_id" (the same ID that is used in "get_gfx()" from
   "modules/gfx.c"), also this routine adds a hook to
   JI_SIGNAL_DESTROY to remove the button from the "icon_buttons"
   list when the widget is free */
void add_gfxicon_to_button(JWidget button, int gfx_id, int icon_align)
{
  button->user_data[3] = (void *)gfx_id;

  jwidget_add_hook(button, JI_WIDGET, button_with_icon_msg_proc, NULL);

  ji_generic_button_set_icon(button, get_gfx(gfx_id));
  ji_generic_button_set_icon_align(button, icon_align);

  jlist_append(icon_buttons, button);
}

void set_gfxicon_in_button(JWidget button, int gfx_id)
{
  button->user_data[3] = (void *)gfx_id;

  ji_generic_button_set_icon(button, get_gfx(gfx_id));

  jwidget_dirty(button);
}

static bool button_with_icon_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_DESTROY)
    jlist_remove(icon_buttons, widget);
  return false;
}

/**********************************************************************/
/* Button style (convert radio or check buttons and draw it like
   normal buttons) */

JWidget radio_button_new(int radio_group, int b1, int b2, int b3, int b4)
{
  JWidget widget = ji_generic_button_new(NULL, JI_RADIO, JI_BUTTON);
  if (widget) {
    jradio_set_group(widget, radio_group);
    jbutton_set_bevel(widget, b1, b2, b3, b4);
    setup_mini_look(widget);
  }
  return widget;
}

JWidget check_button_new(const char *text, int b1, int b2, int b3, int b4)
{
  JWidget widget = ji_generic_button_new(text, JI_CHECK, JI_BUTTON);
  if (widget) {
    jbutton_set_bevel(widget, b1, b2, b3, b4);
    setup_mini_look(widget);
  }
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
    shortcut->command = CommandsModule::instance()->get_command_by_name(command_name);
    shortcut->params = params ? params->clone(): new Params;

    shortcuts->push_back(shortcut);
  }

  shortcut->add_shortcut(shortcut_string);
  return shortcut->accel;
}

JAccel add_keyboard_shortcut_to_change_tool(const char* shortcut_string, Tool* tool)
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

Command* get_command_from_key_message(JMessage msg)
{
  for (std::vector<Shortcut*>::iterator
	 it = shortcuts->begin(); it != shortcuts->end(); ++it) {
    Shortcut* shortcut = *it;

    if (shortcut->type == Shortcut_ExecuteCommand &&
	// TODO why?
	// shortcut->argument.empty() &&
	shortcut->is_key_pressed(msg)) {
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

JAccel get_accel_to_change_tool(Tool* tool)
{
  Shortcut* shortcut = get_keyboard_shortcut_for_tool(tool);
  if (shortcut)
    return shortcut->accel;
  else
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
  jaccel_free(accel);
}

void Shortcut::add_shortcut(const char* shortcut_string)
{
  char buf[256];
  usprintf(buf, "<%s>", shortcut_string);
  jaccel_add_keys_from_string(this->accel, buf);
}

bool Shortcut::is_key_pressed(JMessage msg)
{
  if (accel) {
    return jaccel_check(accel,
			msg->any.shifts,
			msg->key.ascii,
			msg->key.scancode);
  }
  return false;
}

static Shortcut* get_keyboard_shortcut_for_command(const char* command_name, Params* params)
{
  Command* command = CommandsModule::instance()->get_command_by_name(command_name);
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

static Shortcut* get_keyboard_shortcut_for_tool(Tool* tool)
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

/**
 * Adds a routine to be called each 100 milliseconds to monitor
 * whatever you want. It's mainly used to monitor the progress of a
 * file-operation (see @ref fop_operate)
 */
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

/**
 * Removes and frees a previously added monitor.
 */
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
static bool manager_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_QUEUEPROCESSING:
      gui_feedback();

      /* open dropped files */
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

    case JM_KEYPRESSED:
      for (std::vector<Shortcut*>::iterator
	     it = shortcuts->begin(); it != shortcuts->end(); ++it) {
	Shortcut* shortcut = *it;

	if (shortcut->is_key_pressed(msg)) {
	  switch (shortcut->type) {

	    case Shortcut_ChangeTool: {
	      Tool* current_tool = UIContext::instance()->getSettings()->getCurrentTool();
	      Tool* select_this_tool = shortcut->tool;
	      ToolBox* toolbox = App::instance()->get_toolbox();
	      std::vector<Tool*> possibles;

	      // Iterate over all tools
	      for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
		Shortcut* shortcut = get_keyboard_shortcut_for_tool(*it);

		// Collect all tools with the pressed keyboard-shortcut
		if (shortcut && shortcut->is_key_pressed(msg))
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
	      break;
	    }

	    case Shortcut_ExecuteCommand: {
	      Command* command = shortcut->command;

	      // the screen shot is available in everywhere
	      if (strcmp(command->short_name(), CommandId::screen_shot) == 0) {
		UIContext::instance()->execute_command(command, shortcut->params);
		return true;
	      }
	      // all other keys are only available in the main-window
	      else {
		JLink link;

		JI_LIST_FOR_EACH(widget->children, link) {
		  Frame* child = reinterpret_cast<Frame*>(link->data);

		  /* there are a foreground window executing? */
		  if (child->is_foreground()) {
		    break;
		  }
		  /* is it the desktop and the top-window= */
		  else if (child->is_desktop() && child == app_get_top_window()) {
		    /* ok, so we can execute the command represented by the
		       pressed-key in the message... */
		    UIContext::instance()->execute_command(command, shortcut->params);
		    return true;
		  }
		}
	      }
	      break;
	    }

	  }
	  break;
	}
      }
      break;

  }

  return false;
}

// Slot for App::PaletteChange to regenerate graphics when the App palette is changed
static void on_palette_change_signal()
{
  JWidget button;
  JLink link;

  // Regenerate the theme
  ji_regen_theme();

  // Fixup the icons
  JI_LIST_FOR_EACH(icon_buttons, link) {
    button = reinterpret_cast<JWidget>(link->data);
    ji_generic_button_set_icon(button, get_gfx((size_t)button->user_data[3]));
  }
}
