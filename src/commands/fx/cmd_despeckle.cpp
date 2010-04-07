/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "commands/command.h"
#include "commands/fx/effectbg.h"
#include "console.h"
#include "core/cfg.h"
#include "core/core.h"
#include "effect/effect.h"
#include "effect/median.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "ui_context.h"
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

//////////////////////////////////////////////////////////////////////
// despeckle

class DespeckleCommand : public Command
{
public:
  DespeckleCommand();
  Command* clone() const { return new DespeckleCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

DespeckleCommand::DespeckleCommand()
  : Command("despeckle",
	    "Despeckle",
	    CmdRecordableFlag)
{
}

bool DespeckleCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void DespeckleCommand::execute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  JWidget box_target, target_button, button_ok;

  FramePtr window(load_widget("median_blur.xml", "median"));
  get_widgets(window,
	      "width", &entry_width,
	      "height", &entry_height,
	      "preview", &check_preview,
	      "tiled", &check_tiled,
	      "target", &box_target,
	      "button_ok", &button_ok, NULL);

  Effect effect(sprite, "median");
  effect_set_target(&effect, TARGET_RED_CHANNEL |
			     TARGET_GREEN_CHANNEL |
			     TARGET_BLUE_CHANNEL);

  preview = preview_new(&effect);

  target_button = target_button_new(sprite->getImgType(), true);
  target_button_set_target(target_button, effect.target);

  entry_width->setTextf("%d", get_config_int("Median", "Width", 3));
  entry_height->setTextf("%d", get_config_int("Median", "Height", 3));

  if (get_config_bool("Median", "Preview", true))
    jwidget_select(check_preview);

  if (context->getSettings()->getTiledMode() != TILED_NONE)
    jwidget_select(check_tiled);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(entry_width, JI_SIGNAL_ENTRY_CHANGE, width_change_hook, 0);
  HOOK(entry_height, JI_SIGNAL_ENTRY_CHANGE, height_change_hook, 0);
  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(check_preview, JI_SIGNAL_CHECK_CHANGE, preview_change_hook, 0);
  HOOK(check_tiled, JI_SIGNAL_CHECK_CHANGE, tiled_change_hook, 0);
  
  /* default position */
  window->remap_window();
  window->center_window();

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "Median");

  /* open the window */
  window->open_window_fg();

  if (window->get_killer() == button_ok)
    effect_apply_to_target_with_progressbar(&effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "Median");
}

static bool width_change_hook(JWidget widget, void *data)
{
  set_config_int("Median", "Width", widget->getTextInt());
  make_preview();
  return true;
}

static bool height_change_hook(JWidget widget, void *data)
{
  set_config_int("Median", "Height", widget->getTextInt());
  make_preview();
  return true;
}

static bool target_change_hook(JWidget widget, void *data)
{
  effect_set_target(preview_get_effect(preview),
		    target_button_get_target(widget));
  make_preview();
  return false;
}

static bool preview_change_hook(JWidget widget, void *data)
{
  set_config_bool("Median", "Preview", jwidget_is_selected(widget));
  make_preview();
  return false;
}

static bool tiled_change_hook(JWidget widget, void *data)
{
  TiledMode tiled = jwidget_is_selected(widget) ? TILED_BOTH:
							  TILED_NONE;

  // TODO save context in some place, don't use UIContext directly
  UIContext::instance()->getSettings()->setTiledMode(tiled);
  make_preview();
  return false;
}

static void make_preview()
{
  int w, h;

  w = get_config_int("Median", "Width", 3);
  h = get_config_int("Median", "Height", 3);

  // TODO do not use UIContext::instance
  set_median_size(UIContext::instance()->getSettings()->getTiledMode(),
		  MID(1, w, 32), MID(1, h, 32));

  if (jwidget_is_selected (check_preview))
    preview_restart(preview);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_despeckle_command()
{
  return new DespeckleCommand;
}
