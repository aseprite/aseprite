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

#include <stdio.h>

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jentry.h"
#include "jinete/jhook.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "commands/commands.h"
#include "commands/fx/effectbg.h"
#include "console/console.h"
#include "core/cfg.h"
#include "core/core.h"
#include "effect/effect.h"
#include "effect/median.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static JWidget entry_width, entry_height;
static JWidget check_preview, preview;
static JWidget check_tiled;

static bool width_change_hook(JWidget widget, void *data);
static bool height_change_hook(JWidget widget, void *data);
static bool target_change_hook(JWidget widget, void *data);
static bool preview_change_hook(JWidget widget, void *data);
static bool tiled_change_hook(JWidget widget, void *data);
static void make_preview();

static bool cmd_despeckle_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_despeckle_execute(const char *argument)
{
  JWidget window, box_target, target_button, button_ok;
  Image *image;
  Effect *effect;
  char buf[32];

  image = GetImage(current_sprite);
  if (!image)
    return;

  window = load_widget("median.jid", "median");
  if (!window)
    return;

  if (!get_widgets(window,
		   "width", &entry_width,
		   "height", &entry_height,
		   "preview", &check_preview,
		   "tiled", &check_tiled,
		   "target", &box_target,
		   "button_ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  effect = effect_new(current_sprite, "median");
  if (!effect) {
    console_printf(_("Error creating the effect applicator for this sprite\n"));
    jwidget_free(window);
    return;
  }
  effect_set_target(effect, TARGET_RED_CHANNEL |
			    TARGET_GREEN_CHANNEL |
			    TARGET_BLUE_CHANNEL);

  preview = preview_new(effect);

  target_button = target_button_new(current_sprite->imgtype, TRUE);
  target_button_set_target(target_button, effect->target);

  sprintf(buf, "%d", get_config_int("Median", "Width", 3));
  jwidget_set_text(entry_width, buf);
  sprintf(buf, "%d", get_config_int("Median", "Height", 3));
  jwidget_set_text(entry_height, buf);

  if (get_config_bool("Median", "Preview", TRUE))
    jwidget_select(check_preview);

  if (get_tiled_mode())
    jwidget_select(check_tiled);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(entry_width, JI_SIGNAL_ENTRY_CHANGE, width_change_hook, 0);
  HOOK(entry_height, JI_SIGNAL_ENTRY_CHANGE, height_change_hook, 0);
  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);
  HOOK(check_tiled, JI_SIGNAL_CHECK_CHANGE, tiled_change_hook, 0);
  
  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "Median");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    effect_apply_to_target_with_progressbar(effect);
  }

  effect_free(effect);

  /* update editors */
  update_screen_for_sprite(current_sprite);

  /* save window configuration */
  save_window_pos(window, "Median");

  jwidget_free(window);
}

static bool width_change_hook(JWidget widget, void *data)
{
  set_config_int("Median", "Width",
		 strtol(jwidget_get_text(widget), NULL, 10));
  make_preview();
  return TRUE;
}

static bool height_change_hook(JWidget widget, void *data)
{
  set_config_int("Median", "Height",
		 strtol(jwidget_get_text(widget), NULL, 10));
  make_preview();
  return TRUE;
}

static bool target_change_hook(JWidget widget, void *data)
{
  effect_set_target(preview_get_effect(preview),
		    target_button_get_target(widget));
  make_preview();
  return FALSE;
}

static bool preview_change_hook(JWidget widget, void *data)
{
  set_config_bool("Median", "Preview", jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static bool tiled_change_hook(JWidget widget, void *data)
{
  set_tiled_mode(jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview()
{
  int w, h;

  w = get_config_int("Median", "Width", 3);
  h = get_config_int("Median", "Height", 3);

  set_median_size(MID(1, w, 32), MID(1, h, 32));

  if (jwidget_is_selected (check_preview))
    preview_restart(preview);
}

Command cmd_despeckle = {
  CMD_DESPECKLE,
  cmd_despeckle_enabled,
  NULL,
  cmd_despeckle_execute,
  NULL
};
