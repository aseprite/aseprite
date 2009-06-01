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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "commands/fx/effectbg.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "effect/colcurve.h"
#include "effect/effect.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/curvedit.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static Curve *the_curve = NULL;
static JWidget check_preview, preview;

static bool window_msg_proc(JWidget widget, JMessage msg);
static void make_preview();

static bool cmd_color_curve_enabled(const char *argument)
{
  CurrentSprite sprite;
  return sprite != NULL;
}

static void cmd_color_curve_execute(const char *argument)
{
  JWidget window, button_ok;
  JWidget view_curve, curve_editor;
  JWidget box_target, target_button;
  CurrentSprite sprite;
  Image *image;
  Effect *effect;

  if (!the_curve) {
    /* default curve */
    the_curve = curve_new(CURVE_LINEAR);
    curve_add_point(the_curve, curve_point_new(0, 0));
    curve_add_point(the_curve, curve_point_new(255, 255));

    app_add_hook(APP_EXIT,
		 reinterpret_cast<void(*)(void*)>(curve_free),
		 the_curve);
  }

  image = GetImage(sprite);
  if (!image)
    return;

  window = load_widget("colcurv.jid", "color_curve");
  if (!window)
    return;

  if (!get_widgets(window,
		   "preview", &check_preview,
		   "button_ok", &button_ok,
		   "curve", &view_curve,
		   "target", &box_target, NULL)) {
    jwidget_free(window);
    return;
  }

  effect = effect_new(sprite, "color_curve");
  if (!effect) {
    console_printf(_("Error creating the effect applicator for this sprite\n"));
    jwidget_free(window);
    return;
  }
  effect_set_target(effect, TARGET_RED_CHANNEL | TARGET_GREEN_CHANNEL |
			    TARGET_BLUE_CHANNEL | TARGET_ALPHA_CHANNEL);

  preview = preview_new(effect);

  set_color_curve(the_curve);

  curve_editor = curve_editor_new(the_curve, 0, 0, 255, 255);
  target_button = target_button_new(sprite->imgtype, TRUE);
  target_button_set_target(target_button, effect->target);

  if (get_config_bool("ColorCurve", "Preview", TRUE))
    jwidget_select(check_preview);

  jview_attach(view_curve, curve_editor);
  jwidget_set_min_size(view_curve, 128, 64);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  jwidget_add_hook(window, -1, window_msg_proc, NULL);
  
  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "ColorCurve");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    effect_apply_to_target_with_progressbar(effect);
  }

  effect_free(effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "ColorCurve");

  jwidget_free(window);
}

static bool window_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {

      case SIGNAL_CURVE_EDITOR_CHANGE:
	set_color_curve(curve_editor_get_curve(msg->signal.from));
	make_preview();
	break;

      case SIGNAL_TARGET_BUTTON_CHANGE:
	effect_set_target(preview_get_effect(preview),
			  target_button_get_target(msg->signal.from));
	make_preview();
	break;

      case JI_SIGNAL_CHECK_CHANGE:
	set_config_bool("ColorCurve", "Preview",
			jwidget_is_selected(msg->signal.from));
	make_preview();
	break;
    }
  }
  return FALSE;
}

static void make_preview()
{
  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

Command cmd_color_curve = {
  CMD_COLOR_CURVE,
  cmd_color_curve_enabled,
  NULL,
  cmd_color_curve_execute,
  NULL
};
