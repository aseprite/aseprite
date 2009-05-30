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

#include <allegro.h>

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "dialogs/filesel.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "intl/intl.h"

void dialogs_select_language(bool force)
{
  /* only english */
  intl_set_lang("en");
  set_config_bool("Options", "SelectLanguage", FALSE);
}

/**********************************************************************/
/* Options */

static JWidget slider_x, slider_y, check_lockmouse;

static bool slider_mouse_hook(JWidget widget, void *data);

/* shows option dialog */
void dialogs_options()
{
  JWidget window, check_smooth, check_dither;
  JWidget button_ok;
  JWidget move_click2, draw_click2, killer;
  JWidget undo_size_limit;
  int x, y, old_x, old_y;
  char buf[512];

  x = get_config_int("Options", "MouseX", 6);
  y = get_config_int("Options", "MouseY", 6);
  x = MID(-8, x, 8);
  y = MID(-8, y, 8);
  old_x = x;
  old_y = y;

  /* load the window widget */
  window = load_widget("options.jid", "options");
  if (!window)
    return;

  if (!get_widgets(window,
		   "mouse_x", &slider_x,
		   "mouse_y", &slider_y,
		   "lock_axis", &check_lockmouse,
		   "smooth", &check_smooth,
		   "dither", &check_dither,
		   "move_click2", &move_click2,
		   "draw_click2", &draw_click2,
		   "undo_size_limit", &undo_size_limit,
		   "button_ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  jslider_set_value(slider_x, x);
  jslider_set_value(slider_y, y);
  if (get_config_bool("Options", "LockMouse", TRUE))
    jwidget_select(check_lockmouse);
  if (get_config_bool("Options", "MoveClick2", FALSE))
    jwidget_select(move_click2);
  if (get_config_bool("Options", "DrawClick2", FALSE))
    jwidget_select(draw_click2);

  if (get_config_bool("Options", "MoveSmooth", TRUE))
    jwidget_select(check_smooth);

  if (get_config_bool("Options", "Dither", FALSE))
    jwidget_select(check_dither);

  uszprintf(buf, sizeof(buf), "%d", get_config_int("Options", "UndoSizeLimit", 8));
  jwidget_set_text(undo_size_limit, buf);

  HOOK(slider_x, JI_SIGNAL_SLIDER_CHANGE, slider_mouse_hook, NULL);
  HOOK(slider_y, JI_SIGNAL_SLIDER_CHANGE, slider_mouse_hook, NULL);

  jwindow_open_fg(window);
  killer = jwindow_get_killer(window);

  if (killer == button_ok) {
    int undo_size_limit_value;
    
    set_config_bool("Options", "LockMouse", jwidget_is_selected(check_lockmouse));
    set_config_bool("Options", "MoveSmooth", jwidget_is_selected(check_smooth));
    set_config_bool("Options", "MoveClick2", jwidget_is_selected(move_click2));
    set_config_bool("Options", "DrawClick2", jwidget_is_selected(draw_click2));
    
    if (get_config_bool("Options", "Dither", FALSE) != jwidget_is_selected(check_dither)) {
      set_config_bool("Options", "Dither", jwidget_is_selected(check_dither));
      refresh_all_editors();
    }

    undo_size_limit_value = undo_size_limit->text_int();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);

    /* save configuration */
    flush_config_file();
  }
  else {
    /* restore mouse speed */
    set_config_int("Options", "MouseX", old_x);
    set_config_int("Options", "MouseY", old_y);

    set_mouse_speed(8-old_x, 8-old_y);
  }

  jwidget_free(window);
}

static bool slider_mouse_hook(JWidget widget, void *data)
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

/**********************************************************************/
/* setup the mouse speed reading the configuration file */

void _setup_mouse_speed()
{
  int x, y;
  x = get_config_int("Options", "MouseX", 6);
  y = get_config_int("Options", "MouseY", 6);
  x = MID(-8, x, 8);
  y = MID(-8, y, 8);
  set_mouse_speed(8-x, 8-y);
}
