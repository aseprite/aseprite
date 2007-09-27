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

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>

#include "jinete.h"

#include "core/cfg.h"
#include "dialogs/filesel.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "intl/intl.h"

#endif

/* show the language selection dialog */
void dialogs_select_language(bool force)
{
  bool select_language = get_config_bool("Options", "SelectLanguage", TRUE);

  if (force || select_language) {
    JWidget window = jwindow_new("Select Language");
    JWidget box = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS);
    JWidget button_en = jbutton_new("English");
    JWidget button_es = jbutton_new("Español");
    JWidget killer;

    jwidget_add_child(window, box);
    jwidget_add_child(box, button_en);
    jwidget_add_child(box, button_es);

    jwindow_open_fg(window);
    killer = jwindow_get_killer(window);

    /* en */
    if (killer == button_en) {
      intl_set_lang("en");
      set_config_bool("Options", "SelectLanguage", FALSE);
    }
    /* es */
    else if (killer == button_es) {
      intl_set_lang("es");
      set_config_bool("Options", "SelectLanguage", FALSE);
    }

    jwidget_free(window);
  }
}

/**********************************************************************/
/* Options */

static JWidget label_font, slider_x, slider_y, check_lockmouse;

static int slider_mouse_hook (JWidget widget, int user_data);
static void button_font_command (JWidget widget);
static void button_lang_command (JWidget widget);
static void set_label_font_text (void);

/* shows option dialog */
void dialogs_options(void)
{
  JWidget window, move_delay, check_smooth, check_dither;
  JWidget button_font, button_lang, button_ok, button_save;
  JWidget move_click2, draw_click2, askbkpses, killer;
  int x, y, old_x, old_y;
  char *default_font;

  x = get_config_int ("Options", "MouseX", 6);
  y = get_config_int ("Options", "MouseY", 6);
  x = MID (-8, x, 8);
  y = MID (-8, y, 8);
  old_x = x;
  old_y = y;

  /* load the window widget */
  window = load_widget ("options.jid", "options");
  if (!window)
    return;

  if (!get_widgets (window,
		    "mouse_x", &slider_x,
		    "mouse_y", &slider_y,
		    "lock_axis", &check_lockmouse,
		    "move_delay", &move_delay,
		    "smooth", &check_smooth,
		    "dither", &check_dither,
		    "label_font", &label_font,
		    "move_click2", &move_click2,
		    "draw_click2", &draw_click2,
		    "button_font", &button_font,
		    "button_lang", &button_lang,
		    "askbkpses", &askbkpses,
		    "button_ok", &button_ok,
		    "button_save", &button_save, NULL)) {
    jwidget_free (window);
    return;
  }

  jslider_set_value (slider_x, x);
  jslider_set_value (slider_y, y);
  if (get_config_bool ("Options", "LockMouse", TRUE))
    jwidget_select (check_lockmouse);
  if (get_config_bool ("Options", "MoveClick2", FALSE))
    jwidget_select (move_click2);
  if (get_config_bool ("Options", "DrawClick2", FALSE))
    jwidget_select (draw_click2);
  if (get_config_bool ("Options", "AskBkpSes", TRUE))
    jwidget_select (askbkpses);

  jslider_set_value (move_delay,
		       get_config_int ("Options", "MoveDelay", 250));

  if (get_config_bool ("Options", "MoveSmooth", TRUE))
    jwidget_select (check_smooth);

  if (get_config_bool ("Options", "Dither", FALSE))
    jwidget_select (check_dither);

  default_font = jstrdup (get_config_string ("Options", "DefaultFont", ""));
  set_label_font_text ();

  HOOK (slider_x, JI_SIGNAL_SLIDER_CHANGE, slider_mouse_hook, NULL);
  HOOK (slider_y, JI_SIGNAL_SLIDER_CHANGE, slider_mouse_hook, NULL);

  jbutton_add_command (button_font, button_font_command);
  jbutton_add_command (button_lang, button_lang_command);

  jwindow_open_fg (window);
  killer = jwindow_get_killer (window);

  if (killer == button_ok || killer == button_save) {
    set_config_bool ("Options", "LockMouse", jwidget_is_selected (check_lockmouse));
    set_config_int ("Options", "MoveDelay", jslider_get_value (move_delay));
    set_config_bool ("Options", "MoveSmooth", jwidget_is_selected (check_smooth));
    set_config_bool ("Options", "MoveClick2", jwidget_is_selected (move_click2));
    set_config_bool ("Options", "DrawClick2", jwidget_is_selected (draw_click2));
    set_config_bool ("Options", "AskBkpSes", jwidget_is_selected (askbkpses));

    if (get_config_bool ("Options", "Dither", FALSE)
	!= jwidget_is_selected (check_dither)) {
      set_config_bool ("Options", "Dither", jwidget_is_selected (check_dither));
      refresh_all_editors ();
    }

    /* save configuration */
    if (killer == button_save)
      flush_config_file ();
  }
  else {
    /* restore mouse speed */
    set_config_int ("Options", "MouseX", old_x);
    set_config_int ("Options", "MouseY", old_y);

    set_mouse_speed (8-old_x, 8-old_y);

    /* restore default font */
    set_config_string ("Options", "DefaultFont", default_font);
    reload_default_font ();
  }

  jwidget_free (window);
  jfree (default_font);
}

static int slider_mouse_hook (JWidget widget, int user_data)
{
  int x, y;

  if (jwidget_is_selected(check_lockmouse)) {
    x = jslider_get_value(widget);
    y = jslider_get_value(widget);
    jslider_set_value(slider_x, x);
    jslider_set_value(slider_y, y);
  }
  else {
    x = jslider_get_value(slider_x);
    y = jslider_get_value(slider_y);
  }

  set_mouse_speed(8-x, 8-y);

  set_config_int("Options", "MouseX", x);
  set_config_int("Options", "MouseY", y);

  return FALSE;
}

static void button_font_command (JWidget widget)
{
  char *filename;

  filename = GUI_FileSelect(_("Open Font (TTF or Allegro bitmap format)"),
			    get_config_string("Options", "DefaultFont", ""),
			    "pcx,bmp,tga,lbm,ttf");
  if (filename) {
    set_config_string ("Options", "DefaultFont", filename);
    set_label_font_text ();

    reload_default_font ();

    jfree (filename);
  }
}

static void button_lang_command(JWidget widget)
{
  dialogs_select_language(TRUE);
}

static void set_label_font_text(void)
{
  const char *default_font = get_config_string ("Options", "DefaultFont", "");
  char buf[1024];

  usprintf(buf, _("GUI Font: %s"), 
	   *default_font ? get_filename (default_font): _("*Default*"));

  jwidget_set_text(label_font, buf);
}

/**********************************************************************/
/* setup the mouse speed reading the configuration file */

void _setup_mouse_speed(void)
{
  int x, y;
  x = get_config_int("Options", "MouseX", 6);
  y = get_config_int("Options", "MouseY", 6);
  x = MID(-8, x, 8);
  y = MID(-8, y, 8);
  set_mouse_speed(8-x, 8-y);
}
