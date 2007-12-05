/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jslider.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/cfg.h"
#include "core/core.h"
#include "effect/effect.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/preview.h"
#include "widgets/target.h"

#endif

static JWidget check_preview, preview;

static int target_change_hook (JWidget widget, int user_data);
static int preview_change_hook (JWidget widget, int user_data);
static void make_preview (void);

static bool cmd_invert_color_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_invert_color_execute(const char *argument)
{
  JWidget window, box_target, target_button, button_ok;
  Sprite *sprite = current_sprite;
  Image *image;
  Effect *effect;

  image = GetImage();
  if (!image)
    return;

  window = load_widget("invrtcol.jid", "invert_color");
  if (!window)
    return;

  if (!get_widgets(window,
		   "target", &box_target,
		   "preview", &check_preview,
		   "button_ok", &button_ok, NULL)) {
    jwidget_free(window);
    return;
  }

  effect = effect_new(sprite, "invert_color");
  if (!effect) {
    console_printf(_("Error creating the effect applicator for this sprite\n"));
    jwidget_free(window);
    return;
  }

  preview = preview_new(effect);
  target_button = target_button_new(sprite->imgtype, TRUE);

  if (get_config_bool("InvertColor", "Preview", TRUE))
    jwidget_select(check_preview);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "InvertColor");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    effect_apply_to_target(effect);
  }

  effect_free(effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "InvertColor");

  jwidget_free(window);
}

static int target_change_hook(JWidget widget, int user_data)
{
  effect_load_target(preview_get_effect(preview));
  make_preview();
  return FALSE;
}

static int preview_change_hook(JWidget widget, int user_data)
{
  set_config_bool("InvertColor", "Preview", jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview(void)
{
  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

Command cmd_invert_color = {
  CMD_INVERT_COLOR,
  cmd_invert_color_enabled,
  NULL,
  cmd_invert_color_execute,
  NULL
};
