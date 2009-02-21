/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <assert.h>
#include <allegro.h>
#include <allegro/internal/aintern.h>

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/dirs.h"
#include "core/drop_files.h"
#include "dialogs/options.h"
#include "intl/msgids.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/sprite.h"
#include "util/recscr.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

#define REBUILD_RECENT_LIST	2
#define REFRESH_FULL_SCREEN	4

#define MONITOR_TIMER_MSECS	100

/**************************************************************/

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

static int try_depths[] = { 32, 24, 16, 15, 8 };

/**************************************************************/

struct Monitor
{
  /* returns true when the job is done and the monitor can be removed */
  void (*proc)(void *);
  void (*free)(void *);
  void *data;
  bool lock : 1;
  bool deleted : 1;
};

static JWidget manager = NULL;

static int monitor_timer = -1;
static JList monitors;

static bool ji_screen_created = FALSE;

static volatile int next_idle_flags = 0;
static JList icon_buttons;

/* default GUI screen configuration */
static bool double_buffering;
static int screen_scaling;

static Monitor *monitor_new(void (*proc)(void *),
			    void (*free)(void *), void *data);
static void monitor_free(Monitor *monitor);

/* load & save graphics configuration */
static void load_gui_config(int *w, int *h, int *bpp, bool *fullscreen);
static void save_gui_config();

static bool button_with_icon_msg_proc(JWidget widget, JMessage msg);
static bool manager_msg_proc(JWidget widget, JMessage msg);

static void regen_theme_and_fixup_icons(void *data);

/**
 * Used by set_display_switch_callback(SWITCH_IN, ...).
 */
static void display_switch_in_callback()
{
  next_idle_flags |= REFRESH_FULL_SCREEN;
}

END_OF_STATIC_FUNCTION(display_switch_in_callback);

/**
 * Initializes GUI.
 */
int init_module_gui()
{
  int min_possible_dsk_res = 0;
  int c, w, h, bpp, autodetect;
  bool fullscreen;

  /* install timer related stuff */
  if (install_timer() < 0) {
    user_printf(_("Error installing timer handler\n"));
    return -1;
  }

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
  three_finger_flag = FALSE;
#endif
  three_finger_flag = TRUE;	/* TODO remove this line */

  /* set the graphics mode... */
  load_gui_config(&w, &h, &bpp, &fullscreen);

  autodetect = fullscreen ? GFX_AUTODETECT_FULLSCREEN:
			    GFX_AUTODETECT_WINDOWED;

  /* default resolution */
  if (!w || !h) {
    bool has_desktop = FALSE;
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
	  fullscreen = FALSE;
	  w = try_resolutions[c].width;
	  h = try_resolutions[c].height;
	  screen_scaling = try_resolutions[c].scale;
	  break;
	}
      }
    }
    /* full screen */
    else {
      fullscreen = TRUE;
      w = 320;
      h = 200;
      screen_scaling = 1;
    }
  }

  /* default color depth */
  if (!bpp) {
    bpp = desktop_color_depth();
    if (!bpp)
      bpp = 8;
  }

  for (;;) {
    /* original */
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

    if (bpp == 8) {
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

  monitors = jlist_new();

  /* window title */
  set_window_title("Allegro Sprite Editor v" VERSION);

  /* create the default-manager */
  manager = jmanager_new();
  jwidget_add_hook(manager, JI_WIDGET, manager_msg_proc, NULL);

  /* setup the standard jinete theme for widgets */
  ji_set_standard_theme();

  /* set hook to translate strings */
  ji_set_translation_hook(msgids_get);

  /* configure ji_screen */
  gui_setup_screen();

  /* add a hook to display-switch so when the user returns to the
     screen it's completelly refreshed/redrawn */
  LOCK_VARIABLE(next_idle_flags);
  LOCK_FUNCTION(display_switch_in_callback);
  set_display_switch_callback(SWITCH_IN, display_switch_in_callback);

  /* set graphics options for next time */
  save_gui_config();

  /* load the font */
  reload_default_font();

  /* hook for palette change to regenerate the theme */
  app_add_hook(APP_PALETTE_CHANGE, regen_theme_and_fixup_icons, NULL);

  /* icon buttons */
  icon_buttons = jlist_new();

  /* setup mouse */
  _setup_mouse_speed();

  return 0;
}

void exit_module_gui()
{
  JLink link;

  /* destroy monitors */
  JI_LIST_FOR_EACH(monitors, link) {
    monitor_free(reinterpret_cast<Monitor*>(link->data));
  }
  jlist_free(monitors);
  monitors = NULL;

  if (double_buffering) {
    BITMAP *old_bmp = ji_screen;
    ji_set_screen(screen);

    if (ji_screen_created)
      destroy_bitmap(old_bmp);
    ji_screen_created = FALSE;
  }

  jlist_free(icon_buttons);
  icon_buttons = NULL;

  jmanager_free(manager);

  remove_keyboard();
  remove_mouse();
  remove_timer();
}

int guiscale()
{
  return (JI_SCREEN_W > 512 ? 2: 1);
}

static Monitor *monitor_new(void (*proc)(void *),
			    void (*free)(void *), void *data)
{
  Monitor *monitor = jnew(Monitor, 1);
  if (!monitor)
    return NULL;

  monitor->proc = proc;
  monitor->free = free;
  monitor->data = data;
  monitor->lock = FALSE;
  monitor->deleted = FALSE;

  return monitor;
}

static void monitor_free(Monitor *monitor)
{
  if (monitor->free)
    (*monitor->free)(monitor->data);

  jfree(monitor);
}

static void load_gui_config(int *w, int *h, int *bpp, bool *fullscreen)
{
  *w = get_config_int("GfxMode", "Width", 0);
  *h = get_config_int("GfxMode", "Height", 0);
  *bpp = get_config_int("GfxMode", "Depth", 0);
  *fullscreen = get_config_bool("GfxMode", "FullScreen", FALSE);
  screen_scaling = get_config_int("GfxMode", "Scale", 1);
  screen_scaling = MID(1, screen_scaling, 4);
}

static void save_gui_config()
{
  set_config_int("GfxMode", "Width", SCREEN_W);
  set_config_int("GfxMode", "Height", SCREEN_H);
  set_config_int("GfxMode", "Depth", bitmap_color_depth(screen));
  set_config_bool("GfxMode", "FullScreen", gfx_driver->windowed ? FALSE: TRUE);
  set_config_int("GfxMode", "Scale", screen_scaling);
}

int get_screen_scaling()
{
  return screen_scaling;
}

void set_screen_scaling(int scaling)
{
  screen_scaling = scaling;
}

void update_screen_for_sprite(Sprite *sprite)
{
  if (!(ase_mode & MODE_GUI))
    return;

  /* without sprite */
  if (!sprite) {
    /* well, change to the default palette */
    if (set_current_palette(NULL, FALSE)) {
      /* if the palette changes, refresh the whole screen */
      jmanager_refresh_screen();
    }
  }
  /* with a sprite */
  else {
    /* select the palette of the sprite */
    if (set_current_palette(sprite_get_palette(sprite, sprite->frame), FALSE)) {
      /* if the palette changes, refresh the whole screen */
      jmanager_refresh_screen();
    }
    else {
      /* if it's the same palette update only the editors with the sprite */
      update_editors_with_sprite(sprite);
    }
  }

  statusbar_set_text(app_get_statusbar(), -1, "");
}

void gui_run()
{
  jmanager_run(manager);
}

void gui_feedback()
{
  /* menu stuff */
  if (next_idle_flags & REBUILD_RECENT_LIST) {
    if (app_realloc_recent_list())
      next_idle_flags ^= REBUILD_RECENT_LIST;
  }

  if (next_idle_flags & REFRESH_FULL_SCREEN) {
    next_idle_flags ^= REFRESH_FULL_SCREEN;
    update_screen_for_sprite(current_sprite);
  }

  /* record file if is necessary */
  rec_screen_poll();

  /* double buffering? */
  if (double_buffering) {
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
		     0, 0, ji_screen->w, ji_screen->h,
		     0, 0, SCREEN_W, SCREEN_H);
      }
    }
  }
}

/**
 * Sets the ji_screen variable.
 * 
 * This routine should be called everytime you changes the graphics
 * mode.
 */
void gui_setup_screen()
{
  /* double buffering is required when screen scaling is used */
  double_buffering = (screen_scaling > 1);

  /* is double buffering active */
  if (double_buffering) {
    BITMAP *old_bmp = ji_screen;
    ji_set_screen(create_bitmap(SCREEN_W / screen_scaling,
				SCREEN_H / screen_scaling));
    if (ji_screen_created)
      destroy_bitmap(old_bmp);

    ji_screen_created = TRUE;
  }
  else {
    ji_set_screen(screen);
    ji_screen_created = FALSE;
  }

  reload_default_font();

  /* set the configuration */
  save_gui_config();
}

void reload_default_font()
{
  JTheme theme = ji_get_theme();
  const char *user_font;
  DIRS *dirs, *dir;
  char buf[512];

  /* no font for now */

  if (theme->default_font && theme->default_font != font)
    destroy_font(theme->default_font);

  theme->default_font = NULL;

  /* directories */
  dirs = dirs_new();

  user_font = get_config_string("Options", "UserFont", "");
  if ((user_font) && (*user_font))
    dirs_add_path(dirs, user_font);

  usprintf(buf, "fonts/ase%d.pcx", guiscale());
  dirs_cat_dirs(dirs, filename_in_datadir(buf));

  /* try to load the font */
  for (dir=dirs; dir; dir=dir->next) {
    theme->default_font = ji_font_load(dir->path);
    if (theme->default_font) {
      if (ji_font_is_scalable(theme->default_font))
	ji_font_set_size(theme->default_font, 8*guiscale());
      break;
    }
  }

  dirs_free(dirs);

  /* default font: the Allegro one */

  if (!theme->default_font)
    theme->default_font = font;

  /* set all widgets fonts */
  _ji_set_font_of_all_widgets(theme->default_font);
}

void load_window_pos(JWidget window, const char *section)
{
  JRect pos, orig_pos;

  /* default position */
  orig_pos = jwidget_get_rect(window);
  pos = jrect_new_copy(orig_pos);

  /* load configurated position */
  get_config_rect(section, "WindowPos", pos);

  pos->x2 = pos->x1 + MID(jrect_w(orig_pos), jrect_w(pos), JI_SCREEN_W);
  pos->y2 = pos->y1 + MID(jrect_h(orig_pos), jrect_h(pos), JI_SCREEN_H);

  jrect_moveto(pos,
	       MID(0, pos->x1, JI_SCREEN_W-jrect_w(pos)),
	       MID(0, pos->y1, JI_SCREEN_H-jrect_h(pos)));

  jwidget_set_rect(window, pos);

  jrect_free(pos);
  jrect_free(orig_pos);
}

void save_window_pos(JWidget window, const char *section)
{
  set_config_rect(section, "WindowPos", window->rc);
}

JWidget load_widget(const char *filename, const char *name)
{
  JWidget widget;
  DIRS *it, *dirs;
  char buf[512];
  bool found = FALSE;

  dirs = dirs_new();

  usprintf(buf, "jids/%s", filename);

  dirs_add_path(dirs, filename);
  dirs_cat_dirs(dirs, filename_in_datadir(buf));

  for (it=dirs; it; it=it->next) {
    if (exists(it->path)) {
      ustrcpy(buf, it->path);
      found = TRUE;
      break;
    }
  }

  dirs_free(dirs);

  if (!found) {
    console_printf(_("File not found: \"%s\"\n"), filename);
    return NULL;
  }

  widget = ji_load_widget(buf, name);
  if (!widget)
    console_printf(_("Error loading widget: \"%s\"\n"), name);

  return widget;
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

  return FALSE;
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
bool get_widgets(JWidget window, ...)
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
    if (!*widget) {
      console_printf(_("Widget %s not found.\n"), name);
      return FALSE;
    }
  }
  va_end(ap);

  return TRUE;
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
  return FALSE;
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
  }
  return widget;
}

JWidget check_button_new(const char *text, int b1, int b2, int b3, int b4)
{
  JWidget widget = ji_generic_button_new(text, JI_CHECK, JI_BUTTON);
  if (widget) {
    jbutton_set_bevel(widget, b1, b2, b3, b4);
  }
  return widget;
}

/**
 * Adds a routine to be called each 100 milliseconds to monitor
 * whatever you want. It's mainly used to monitor the progress of a
 * file-operation (see @ref fop_operate)
 */
Monitor *add_gui_monitor(void (*proc)(void *),
			 void (*free)(void *), void *data)
{
  Monitor *monitor = monitor_new(proc, free, data);

  jlist_append(monitors, monitor);

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
  JLink link = jlist_find(monitors, monitor);
  assert(link != NULL);

  if (!monitor->lock)
    monitor_free(monitor);
  else
    monitor->deleted = TRUE;

  jlist_delete_link(monitors, link);
  if (jlist_empty(monitors))
    jmanager_stop_timer(monitor_timer);
}

/**********************************************************************/
/* manager event handler */

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
	JLink link, next;
	JI_LIST_FOR_EACH_SAFE(monitors, link, next) {
	  Monitor* monitor = reinterpret_cast<Monitor*>(link->data);

	  /* is the monitor not lock? */
	  if (!monitor->lock) {
	    /* call the monitor procedure */
	    monitor->lock = TRUE;
	    (*monitor->proc)(monitor->data);
	    monitor->lock = FALSE;

	    if (monitor->deleted)
	      monitor_free(monitor);
	  }
	}

	/* is monitors empty? we can stop the timer so */
	if (jlist_empty(monitors))
	  jmanager_stop_timer(monitor_timer);
      }
      break;

    case JM_KEYPRESSED: {
      /* check for commands */
      Command *command = command_get_by_key(msg);
      if (!command) {
	/* check for tools */
	Tool *tool = get_tool_by_key(msg);
	if (tool != NULL)
	  select_tool(tool);
	break;
      }

      /* the screen shot is available in everywhere */
      if (strcmp(command->name, CMD_SCREEN_SHOT) == 0) {
	if (command_is_enabled(command, NULL)) {
	  command_execute(command, NULL);
	  return TRUE;
	}
      }
      /* all other keys are only available in the main-window */
      else {
	JWidget child;
	JLink link;

	JI_LIST_FOR_EACH(widget->children, link) {
	  child = reinterpret_cast<JWidget>(link->data);

	  /* there are a foreground window executing? */
	  if (jwindow_is_foreground(child)) {
	    break;
	  }
	  /* is it the desktop and the top-window= */
	  else if (jwindow_is_desktop(child) && child == app_get_top_window()) {
	    /* ok, so we can execute the command represented by the
	       pressed-key in the message... */
	    if (command_is_enabled(command, NULL)) {
	      command_execute(command, NULL);
	      return TRUE;
	    }
	    break;
	  }
	}
      }
      break;
    }

  }

  return FALSE;
}

/**********************************************************************/
/* graphics */

static void regen_theme_and_fixup_icons(void *data)
{
  JWidget button;
  JLink link;

  /* regenerate the theme */
  ji_regen_theme();

  /* fixup the icons */
  JI_LIST_FOR_EACH(icon_buttons, link) {
    button = reinterpret_cast<JWidget>(link->data);
    ji_generic_button_set_icon(button, get_gfx((size_t)button->user_data[3]));
  }
}
