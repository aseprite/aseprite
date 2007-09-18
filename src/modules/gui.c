/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

/* #define GUI_DOUBLE_BUFFERED */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete.h"
#include "jinete/intern.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/dirs.h"
#include "dialogs/gfxsel.h"
#include "dialogs/options.h"
#include "dialogs/filmedit.h"
#include "intl/msgids.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "script/script.h"
#include "util/recscr.h"
#include "util/scrshot.h"
#include "widgets/editor.h"

#endif

#define REBUILD_LOCK		1
#define REBUILD_ROOT_MENU	2
#define REBUILD_SPRITE_LIST	4
#define REBUILD_RECENT_LIST	8

static JWidget manager = NULL;

static int next_idle_flags = 0;
static JList icon_buttons;

static bool button_with_icon_msg_proc(JWidget widget, JMessage msg);
static bool manager_msg_proc(JWidget widget, JMessage msg);

static void regen_theme_and_fixup_icons(void);

/**
 * Used by set_display_switch_callback(SWITCH_IN, ...).
 */
static void display_switch_in_callback()
{
  GUI_Refresh(current_sprite);
}

/**
 * Initializes GUI.
 */
int init_module_gui(void)
{
  int w, h, bpp, autodetect;
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

  /* set the graphics mode... */
  load_gui_config(&w, &h, &bpp, &fullscreen);

  autodetect = fullscreen ? GFX_AUTODETECT_FULLSCREEN:
			    GFX_AUTODETECT_WINDOWED;

  if (!w) w = DEFAULT_GFX_WIDTH;
  if (!h) h = DEFAULT_GFX_HEIGHT;
  if (!bpp) {
    bpp = desktop_color_depth();
    if (!bpp)
      bpp = DEFAULT_GFX_DEPTH;
  }

  /* try original mode */
  set_color_depth(bpp);
  if (set_gfx_mode(autodetect, w, h, 0, 0) < 0) {
    /* try the same resolution but with 8 bpp */
    set_color_depth(bpp = 8);
    if (set_gfx_mode(autodetect, w, h, 0, 0) < 0) {
      /* try 320x200 in 8 bpp */
      if (set_gfx_mode(autodetect, w = 320, h = 200, 0, 0) < 0) {
	user_printf(_("Error setting graphics mode\n%s\n"
		      "Try \"ase -res WIDTHxHEIGHTxBPP\"\n"), allegro_error);
	return -1;
      }
    }
  }

  /* create the default-manager */
  manager = jmanager_new();
  jwidget_add_hook(manager, JI_WIDGET, manager_msg_proc, NULL);

  /* setup the standard jinete theme for widgets */
  ji_set_standard_theme();

  /* set hook to translate strings */
  ji_set_translation_hook((void *)msgids_get);

  /* configure ji_screen */
  gui_setup_screen();

  /* add a hook to display-switch so when the user returns to the
     screen it's completelly refreshed/redrawn */
  set_display_switch_callback(SWITCH_IN, display_switch_in_callback);

  /* set graphics options for next time */
  save_gui_config();

  /* load the font */
  reload_default_font();

  /* hook for palette change to regenerate the theme */
  hook_palette_changes(regen_theme_and_fixup_icons);

  /* icon buttons */
  icon_buttons = jlist_new();

  /* setup mouse */
  _setup_mouse_speed();

  return 0;
}

void exit_module_gui(void)
{
#ifdef GUI_DOUBLE_BUFFERED
  BITMAP *old_bmp = ji_screen;
  ji_set_screen(screen);
  if (old_bmp && old_bmp != screen)
    destroy_bitmap(old_bmp);
#endif

  jlist_free(icon_buttons);

  destroy_gfx_cards();
  unhook_palette_changes(regen_theme_and_fixup_icons);

  jmanager_free(manager);

  remove_keyboard();
  remove_mouse();
  remove_timer();
}

void load_gui_config(int *w, int *h, int *bpp, bool *fullscreen)
{
  *w = get_config_int("GfxMode", "Width", 0);
  *h = get_config_int("GfxMode", "Height", 0);
  *bpp = get_config_int("GfxMode", "Depth", 0);
  *fullscreen = get_config_bool("GfxMode", "FullScreen", TRUE);
}

void save_gui_config(void)
{
  set_config_int("GfxMode", "Width", JI_SCREEN_W);
  set_config_int("GfxMode", "Height", JI_SCREEN_H);
  set_config_int("GfxMode", "Depth", bitmap_color_depth(screen));
  set_config_bool("GfxMode", "FullScreen", gfx_driver->windowed ? FALSE: TRUE);
}

void GUI_Refresh(Sprite *sprite)
{
  if (!(ase_mode & MODE_GUI))
    return;

  if (!sprite) {
    if (set_current_palette(NULL, FALSE))
      jmanager_refresh_screen();
  }
  else {
    if (set_current_palette(sprite_get_palette(sprite, sprite->frpos), FALSE))
      jmanager_refresh_screen();
    else
      update_editors_with_sprite(sprite);
  }
}

void run_gui(void)
{
  jmanager_run (manager);
}

void gui_feedback(void)
{
  /* menu stuff */
  if (!(next_idle_flags & REBUILD_LOCK)) {
    if (next_idle_flags & REBUILD_ROOT_MENU) {
      next_idle_flags ^= REBUILD_ROOT_MENU;
      load_root_menu();

      next_idle_flags |= REBUILD_SPRITE_LIST;
      next_idle_flags |= REBUILD_RECENT_LIST;
    }

    if (next_idle_flags & REBUILD_SPRITE_LIST) {
      next_idle_flags ^= REBUILD_SPRITE_LIST;
      app_realloc_sprite_list();
    }

    if (next_idle_flags & REBUILD_RECENT_LIST) {
      next_idle_flags ^= REBUILD_RECENT_LIST;
      app_realloc_recent_list();
    }
  }

  /* record file if is necessary */
  rec_screen_poll();

#ifdef GUI_DOUBLE_BUFFERED
  if (JI_SCREEN_W == SCREEN_W && JI_SCREEN_H == SCREEN_H) {
    blit(ji_screen, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
  }
  else {
    stretch_blit(ji_screen, screen,
		 0, 0, ji_screen->w, ji_screen->h,
		 0, 0, SCREEN_W, SCREEN_H);
  }
#endif

  /* don't eat CPU... rest some time */
  rest(1);
}

void gui_setup_screen(void)
{
#ifdef GUI_DOUBLE_BUFFERED
  BITMAP *old_bmp = ji_screen;
  ji_set_screen(create_bitmap(SCREEN_W, SCREEN_H));
  if (old_bmp && old_bmp != screen)
    destroy_bitmap(old_bmp);
#else
  ji_set_screen(screen);
#endif
}

void reload_default_font(void)
{
  JTheme theme = ji_get_theme ();
  const char *default_font;
  DIRS *dirs, *dir;

  /* no font for now */

  if (theme->default_font && theme->default_font != font)
    destroy_font (theme->default_font);

  theme->default_font = NULL;

  /* directories */
  dirs = dirs_new ();

  default_font = get_config_string ("Options", "DefaultFont", "");
  if ((default_font) && (*default_font))
    dirs_add_path (dirs, default_font);

  /* big font */
  if (JI_SCREEN_W > 320)
    dirs_cat_dirs (dirs, filename_in_datadir ("fonts/default2.pcx"));
  /* tiny font */
  else
    dirs_cat_dirs (dirs, filename_in_datadir ("fonts/default.pcx"));

  /* try to load the font */
  for (dir=dirs; dir; dir=dir->next) {
    theme->default_font = ji_font_load (dir->path);
    if (theme->default_font) {
      if (ji_font_is_scalable (theme->default_font)) {
	ji_font_set_size (theme->default_font,
			  (JI_SCREEN_W > 320) ? 14: 8);
      }
      break;
    }
  }

  dirs_free (dirs);

  /* default font: the Allegro one */

  if (!theme->default_font)
    theme->default_font = font;

  /* set all widgets fonts */
  _ji_set_font_of_all_widgets (theme->default_font);
}

void load_window_pos(JWidget window, const char *section)
{
  JRect pos, orig_pos;

  /* default position */
  orig_pos = jwidget_get_rect (window);
  pos = jrect_new_copy (orig_pos);

  /* load configurated position */
  get_config_rect (section, "WindowPos", pos);

  pos->x2 = pos->x1 + MID(jrect_w(orig_pos), jrect_w(pos), JI_SCREEN_W);
  pos->y2 = pos->y1 + MID(jrect_h(orig_pos), jrect_h(pos), JI_SCREEN_H);

  jrect_moveto (pos,
		MID(0, pos->x1, JI_SCREEN_W-jrect_w(pos)),
		MID(0, pos->y1, JI_SCREEN_H-jrect_h(pos)));

  jwidget_set_rect (window, pos);

  jrect_free (pos);
  jrect_free (orig_pos);
}

void save_window_pos(JWidget window, const char *section)
{
  set_config_rect (section, "WindowPos", window->rc);
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

  if (found)
    widget = ji_load_widget(buf, name);
  else
    widget = NULL;

  if (!found) {
    console_printf(_("File not found: \"%s\"\n"), filename);
    return NULL;
  }

  widget = ji_load_widget (buf, name);
  if (!widget)
    console_printf(_("Error loading widget: \"%s\"\n"), name);
  return widget;
}

void rebuild_lock(void)
{
  next_idle_flags |= REBUILD_LOCK;
}

void rebuild_unlock(void)
{
  next_idle_flags &= ~REBUILD_LOCK;
}

void rebuild_root_menu(void)
{
  next_idle_flags |= REBUILD_ROOT_MENU;
}

void rebuild_sprite_list(void)
{
  next_idle_flags |= REBUILD_SPRITE_LIST;
}

void rebuild_recent_list(void)
{
  next_idle_flags |= REBUILD_RECENT_LIST;
}

/**********************************************************************/
/* hook signals */

typedef struct HookData {
  int signal_num;
  int (*signal_handler) (JWidget widget, int user_data);
  int user_data;
} HookData;

static int hook_type (void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

static bool hook_handler (JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      jfree (jwidget_get_data (widget, hook_type ()));
      break;

    case JM_SIGNAL: {
      HookData *hook_data = jwidget_get_data (widget, hook_type ());
      if (hook_data->signal_num == msg->signal.num)
	return (*hook_data->signal_handler) (widget, hook_data->user_data);
      break;
    }
  }

  return FALSE;
}

void hook_signal(JWidget widget,
		 int signal_num,
		 int (*signal_handler) (JWidget widget, int user_data),
		 int user_data)
{
  HookData *hook_data = jnew (HookData, 1);

  hook_data->signal_num = signal_num;
  hook_data->signal_handler = signal_handler;
  hook_data->user_data = user_data;

  jwidget_add_hook (widget, hook_type (), hook_handler, hook_data);
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

  va_start (ap, window);
  while ((name = va_arg (ap, char *))) {
    widget = va_arg (ap, JWidget *);
    if (!widget)
      break;

    *widget = jwidget_find_name (window, name);
    if (!*widget) {
      console_printf (_("Widget %s not found.\n"), name);
      return FALSE;
    }
  }
  va_end (ap);

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

void set_gfxicon_in_button (JWidget button, int gfx_id)
{
  button->user_data[3] = (void *)gfx_id;

  ji_generic_button_set_icon(button, get_gfx(gfx_id));

  jwidget_dirty(button);
}

static bool button_with_icon_msg_proc (JWidget widget, JMessage msg)
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

#if 0
static int button_style_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool button_style_msg_proc(JWidget widget, JMessage msg)
{
  JHook hook = (JHook)jwidget_get_data(widget, button_style_type());

  switch (msg->type) {

    case JM_DESTROY:
      (*hook->msg_proc)(widget, msg);
      jfree(hook);
      break;

    case JM_REQSIZE:
      return (*hook->msg_proc)(widget, msg);
  }

  return FALSE;
}

void change_to_button_style(JWidget widget, int b1, int b2, int b3, int b4)
{
  JWidget button = jbutton_new(NULL);
  JHook hook = jwidget_get_hook(button, JI_BUTTON);

  /* setup button bevel */
  jbutton_set_bevel(button, b1, b2, b3, b4);

  /* steal JI_BUTTON hook */
  _jwidget_remove_hook(button, hook);

  /* put the JI_BUTTON hook data in the widget (to get it with
     jwidget_get_data) */
  jwidget_add_hook(widget, JI_BUTTON, NULL, hook->data);

  /* put a cusomized hook to filter only some messages to the real
     JI_BUTTON hook msg_proc */
  jwidget_add_hook (widget, button_style_type (), button_style_msg_proc, hook);

  /* setup widget geometry */
  widget->align = button->align;
  widget->border_width = button->border_width;
  widget->draw_method = button->draw_method;

  /* jwidget_set_border (widget, 2, 2, 2, 2); */

  /* the data will be free after */
  jwidget_free (button);
}
#endif


/**********************************************************************/
/* manager event handler */

static bool manager_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_IDLE:
      gui_feedback();
      break;

    case JM_CHAR:
      if (check_for_accel(ACCEL_FOR_SCREENSHOT, msg)) {
	screen_shot();
	return TRUE;
      }
      else if (check_for_accel(ACCEL_FOR_FILMEDITOR, msg)) {
	if (current_sprite) {
	  JWidget child;
	  JLink link;
	  bool dofilm = FALSE;

	  JI_LIST_FOR_EACH(widget->children, link) {
	    child = link->data;

	    if (jwindow_is_foreground(child)) {
 	      break;
	    }
	    else if (jwindow_is_desktop(child) && child == app_get_top_window()) {
	      dofilm = TRUE;
	      break;
	    }
	  }

	  if (dofilm) {
	    switch_between_film_and_sprite_editor();
	    return TRUE;
	  }
	}
      }
      break;
  }

  return FALSE;
}

/**********************************************************************/
/* graphics */

static void regen_theme_and_fixup_icons(void)
{
  JWidget button;
  JLink link;

  /* regenerate the theme */
  ji_regen_theme();

  /* fixup the icons */
  JI_LIST_FOR_EACH(icon_buttons, link) {
    button = link->data;
    ji_generic_button_set_icon(button, get_gfx((int)button->user_data[3]));
  }
}
