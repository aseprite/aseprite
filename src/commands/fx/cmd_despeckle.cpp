/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "base/bind.h"
#include "gui/button.h"
#include "gui/entry.h"
#include "gui/frame.h"
#include "gui/jhook.h"
#include "gui/widget.h"

#include "commands/command.h"
#include "commands/fx/effectbg.h"
#include "console.h"
#include "core/cfg.h"
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
static JWidget preview;
static CheckBox* check_preview;
static CheckBox* check_tiled;

static bool width_change_hook(JWidget widget, void *data);
static bool height_change_hook(JWidget widget, void *data);
static bool target_change_hook(JWidget widget, void *data);
static void preview_change_hook(JWidget widget);
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
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DespeckleCommand::DespeckleCommand()
  : Command("Despeckle",
	    "Despeckle",
	    CmdRecordableFlag)
{
}

bool DespeckleCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void DespeckleCommand::onExecute(Context* context)
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
    check_preview->setSelected(true);

  if (context->getSettings()->getTiledMode() != TILED_NONE)
    check_tiled->setSelected(true);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(entry_width, JI_SIGNAL_ENTRY_CHANGE, width_change_hook, 0);
  HOOK(entry_height, JI_SIGNAL_ENTRY_CHANGE, height_change_hook, 0);
  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  HOOK(check_tiled, JI_SIGNAL_CHECK_CHANGE, tiled_change_hook, 0);

  check_preview->Click.connect(Bind<void>(&preview_change_hook, check_preview));
  
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

static void preview_change_hook(JWidget widget)
{
  set_config_bool("Median", "Preview", widget->isSelected());
  make_preview();
}

static bool tiled_change_hook(JWidget widget, void *data)
{
  TiledMode tiled = widget->isSelected() ? TILED_BOTH:
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

  if (check_preview->isSelected())
    preview_restart(preview);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDespeckleCommand()
{
  return new DespeckleCommand;
}
