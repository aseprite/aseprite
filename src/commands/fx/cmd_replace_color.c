/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007, 2008  David A. Capello
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

#include <stdio.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "effect/effect.h"
#include "effect/replcol.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"
#include "widgets/preview.h"
#include "widgets/target.h"

#endif

static JWidget button_color1, button_color2;
static JWidget slider_fuzziness, check_preview;
static JWidget preview;

static int button_1_select_hook(JWidget widget, int user_data);
static int button_2_select_hook(JWidget widget, int user_data);
static int color_change_hook(JWidget widget, int user_data);
static int target_change_hook(JWidget widget, int user_data);
static int slider_change_hook(JWidget widget, int user_data);
static int preview_change_hook(JWidget widget, int user_data);
static void make_preview(void);

static bool cmd_replace_color_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_replace_color_execute(const char *argument)
{
  JWidget window, color_buttons_box;
  JWidget button1_1, button1_2;
  JWidget button2_1, button2_2;
  JWidget box_target, target_button;
  JWidget button_ok;
  Image *image;
  Effect *effect;

  image = GetImage();
  if (!image)
    return;

  window = load_widget("replcol.jid", "replace_color");
  if (!window)
    return;

  if (!get_widgets(window,
		   "color_buttons_box", &color_buttons_box,
		   "button1_1", &button1_1,
		   "button1_2", &button1_2,
		   "button2_1", &button2_1,
		   "button2_2", &button2_2,
		   "preview", &check_preview,
		   "fuzziness", &slider_fuzziness,
		   "target", &box_target,
		   "button_ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  effect = effect_new(current_sprite, "replace_color");
  if (!effect) {
    console_printf(_("Error creating the effect applicator for this sprite\n"));
    jwidget_free(window);
    return;
  }

  preview = preview_new(effect);

  button_color1 = color_button_new
    (get_config_string("ReplaceColor", "Color1",
		       color_bar_get_color(app_get_color_bar(), 0)),
     current_sprite->imgtype);

  button_color2 = color_button_new
    (get_config_string("ReplaceColor", "Color2",
		       color_bar_get_color(app_get_color_bar(), 1)),
     current_sprite->imgtype);

  target_button = target_button_new(current_sprite->imgtype, FALSE);

  jslider_set_value(slider_fuzziness,
		    get_config_int("ReplaceColor", "Fuzziness", 0));
  if (get_config_bool("ReplaceColor", "Preview", TRUE))
    jwidget_select(check_preview);

  jwidget_add_child(color_buttons_box, button_color1);
  jwidget_add_child(color_buttons_box, button_color2);
  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(button1_1, JI_SIGNAL_BUTTON_SELECT, button_1_select_hook, 1);
  HOOK(button1_2, JI_SIGNAL_BUTTON_SELECT, button_2_select_hook, 1);
  HOOK(button2_1, JI_SIGNAL_BUTTON_SELECT, button_1_select_hook, 2);
  HOOK(button2_2, JI_SIGNAL_BUTTON_SELECT, button_2_select_hook, 2);
  HOOK(button_color1, SIGNAL_COLOR_BUTTON_CHANGE, color_change_hook, 1);
  HOOK(button_color2, SIGNAL_COLOR_BUTTON_CHANGE, color_change_hook, 2);
  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(slider_fuzziness, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "ReplaceColor");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    effect_apply_to_target(effect);
  }

  effect_free(effect);

  /* update editors */
  update_screen_for_sprite(current_sprite);

  /* save window configuration */
  save_window_pos(window, "ReplaceColor");

  jwidget_free(window);
}

static int button_1_select_hook(JWidget widget, int user_data)
{
  JWidget w = user_data == 1 ? button_color1: button_color2;

  color_button_set_color(w, color_bar_get_color(app_get_color_bar(), 0));
  color_change_hook(w, user_data);

  return TRUE;
}

static int button_2_select_hook(JWidget widget, int user_data)
{
  JWidget w = user_data == 1 ? button_color1: button_color2;

  color_button_set_color(w, color_bar_get_color(app_get_color_bar(), 1));
  color_change_hook(w, user_data);

  return TRUE;
}

static int color_change_hook(JWidget widget, int user_data)
{
  char buf[64];

  sprintf(buf, "Color%d", user_data);
  set_config_string("ReplaceColor", buf, color_button_get_color(widget));

  make_preview();
  return FALSE;
}

static int target_change_hook(JWidget widget, int user_data)
{
  effect_load_target(preview_get_effect(preview));
  make_preview();
  return FALSE;
}

static int slider_change_hook(JWidget widget, int user_data)
{
  set_config_int("ReplaceColor", "Fuzziness", jslider_get_value(widget));
  make_preview();
  return FALSE;
}

static int preview_change_hook(JWidget widget, int user_data)
{
  set_config_bool("ReplaceColor", "Preview", jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview(void)
{
  Effect *effect = preview_get_effect(preview);
  const char *from, *to;
  int fuzziness;

  from = get_config_string("ReplaceColor", "Color1", "mask");
  to = get_config_string("ReplaceColor", "Color2", "mask");
  fuzziness = get_config_int("ReplaceColor", "Fuzziness", 0);

  set_replace_colors(get_color_for_image(effect->dst->imgtype, from),
		      get_color_for_image(effect->dst->imgtype, to),
		      MID(0, fuzziness, 255));

  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

Command cmd_replace_color = {
  CMD_REPLACE_COLOR,
  cmd_replace_color_enabled,
  NULL,
  cmd_replace_color_execute,
  NULL
};
