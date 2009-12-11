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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/fx/effectbg.h"
#include "console.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "effect/effect.h"
#include "effect/replcol.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static JWidget button_color1, button_color2;
static JWidget slider_fuzziness, check_preview;
static JWidget preview;

static bool color_change_hook(JWidget widget, void *data);
static bool target_change_hook(JWidget widget, void *data);
static bool slider_change_hook(JWidget widget, void *data);
static bool preview_change_hook(JWidget widget, void *data);
static void make_preview();

//////////////////////////////////////////////////////////////////////
// replace_color

class ReplaceColorCommand : public Command
{
public:
  ReplaceColorCommand();
  Command* clone() const { return new ReplaceColorCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

ReplaceColorCommand::ReplaceColorCommand()
  : Command("replace_color",
	    "Replace Color",
	    CmdRecordableFlag)
{
}

bool ReplaceColorCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void ReplaceColorCommand::execute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  JWidget color_buttons_box;
  JWidget box_target, target_button;
  JWidget button_ok;

  JWidgetPtr window(load_widget("replcol.jid", "replace_color"));
  get_widgets(window,
	      "color_buttons_box", &color_buttons_box,
	      "preview", &check_preview,
	      "fuzziness", &slider_fuzziness,
	      "target", &box_target,
	      "button_ok", &button_ok, NULL);

  Effect effect(sprite, "replace_color");
  effect_set_target(&effect, TARGET_RED_CHANNEL |
			     TARGET_GREEN_CHANNEL |
			     TARGET_BLUE_CHANNEL |
			     TARGET_ALPHA_CHANNEL);
  preview = preview_new(&effect);

  button_color1 = colorbutton_new
    (get_config_color("ReplaceColor", "Color1",
		      colorbar_get_fg_color(app_get_colorbar())),
     sprite->imgtype);

  button_color2 = colorbutton_new
    (get_config_color("ReplaceColor", "Color2",
		      colorbar_get_bg_color(app_get_colorbar())),
     sprite->imgtype);

  target_button = target_button_new(sprite->imgtype, FALSE);
  target_button_set_target(target_button, effect.target);

  jslider_set_value(slider_fuzziness,
		    get_config_int("ReplaceColor", "Fuzziness", 0));
  if (get_config_bool("ReplaceColor", "Preview", TRUE))
    jwidget_select(check_preview);

  jwidget_add_child(color_buttons_box, button_color1);
  jwidget_add_child(color_buttons_box, button_color2);
  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(button_color1, SIGNAL_COLORBUTTON_CHANGE, color_change_hook, 1);
  HOOK(button_color2, SIGNAL_COLORBUTTON_CHANGE, color_change_hook, 2);
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

  if (jwindow_get_killer(window) == button_ok)
    effect_apply_to_target_with_progressbar(&effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "ReplaceColor");
}

static bool color_change_hook(JWidget widget, void *data)
{
  char buf[64];

  sprintf(buf, "Color%u", (size_t)data);
  set_config_color("ReplaceColor", buf, colorbutton_get_color(widget));

  make_preview();
  return FALSE;
}

static bool target_change_hook(JWidget widget, void *data)
{
  effect_set_target(preview_get_effect(preview),
		    target_button_get_target(widget));
  make_preview();
  return FALSE;
}

static bool slider_change_hook(JWidget widget, void *data)
{
  set_config_int("ReplaceColor", "Fuzziness", jslider_get_value(widget));
  make_preview();
  return FALSE;
}

static bool preview_change_hook(JWidget widget, void *data)
{
  set_config_bool("ReplaceColor", "Preview", jwidget_is_selected(widget));
  make_preview();
  return FALSE;
}

static void make_preview()
{
  Sprite* sprite = preview_get_effect(preview)->sprite;
  color_t from, to;
  int fuzziness;

  from = get_config_color("ReplaceColor", "Color1", color_mask());
  to = get_config_color("ReplaceColor", "Color2", color_mask());
  fuzziness = get_config_int("ReplaceColor", "Fuzziness", 0);

  set_replace_colors(get_color_for_layer(sprite->layer, from),
		     get_color_for_layer(sprite->layer, to),
		     MID(0, fuzziness, 255));

  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_replace_color_command()
{
  return new ReplaceColorCommand;
}
