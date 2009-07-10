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

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jslider.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "commands/commands.h"
#include "commands/fx/effectbg.h"
#include "console/console.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "effect/effect.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static JWidget check_preview, preview;

static bool target_change_hook(JWidget widget, void *data);
static bool preview_change_hook(JWidget widget, void *data);
static void make_preview();

static bool cmd_invert_color_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL;
}

static void cmd_invert_color_execute(const char *argument)
{
  JWidget box_target, target_button, button_ok;
  const CurrentSpriteReader sprite;

  JWidgetPtr window(load_widget("invrtcol.jid", "invert_color"));
  get_widgets(window,
	      "target", &box_target,
	      "preview", &check_preview,
	      "button_ok", &button_ok, NULL);

  Effect effect(sprite, "invert_color");
  effect_set_target(&effect, TARGET_RED_CHANNEL |
			     TARGET_GREEN_CHANNEL |
			     TARGET_BLUE_CHANNEL);

  preview = preview_new(&effect);
  target_button = target_button_new(sprite->imgtype, TRUE);
  target_button_set_target(target_button, effect.target);

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

  if (jwindow_get_killer(window) == button_ok)
    effect_apply_to_target_with_progressbar(&effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "InvertColor");
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
  set_config_bool("InvertColor", "Preview", jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview()
{
  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

Command cmd_invert_color = {
  CMD_INVERT_COLOR,
  cmd_invert_color_enabled,
  NULL,
  cmd_invert_color_execute,
};
