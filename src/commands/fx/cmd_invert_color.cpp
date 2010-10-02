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

#include "base/bind.h"
#include "gui/jbox.h"
#include "gui/button.h"
#include "gui/jhook.h"
#include "gui/label.h"
#include "gui/jslider.h"
#include "gui/jwidget.h"
#include "gui/frame.h"

#include "app/color.h"
#include "commands/command.h"
#include "commands/fx/effectbg.h"
#include "console.h"
#include "core/cfg.h"
#include "effect/effect.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/color_button.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static CheckBox* check_preview;
static JWidget preview;

static bool target_change_hook(JWidget widget, void *data);
static void preview_change_hook(Widget* widget);
static void make_preview();

//////////////////////////////////////////////////////////////////////
// invert_color

class InvertColorCommand : public Command
{
public:
  InvertColorCommand();
  Command* clone() const { return new InvertColorCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

InvertColorCommand::InvertColorCommand()
  : Command("invert_color",
	    "Invert Color",
	    CmdRecordableFlag)
{
}

bool InvertColorCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void InvertColorCommand::onExecute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  JWidget box_target, target_button, button_ok;

  FramePtr window(load_widget("invert_color.xml", "invert_color"));
  get_widgets(window,
	      "target", &box_target,
	      "preview", &check_preview,
	      "button_ok", &button_ok, NULL);

  Effect effect(sprite, "invert_color");
  effect_set_target(&effect, TARGET_RED_CHANNEL |
			     TARGET_GREEN_CHANNEL |
			     TARGET_BLUE_CHANNEL);

  preview = preview_new(&effect);
  target_button = target_button_new(sprite->getImgType(), true);
  target_button_set_target(target_button, effect.target);

  if (get_config_bool("InvertColor", "Preview", true))
    check_preview->setSelected(true);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  HOOK(target_button, SIGNAL_TARGET_BUTTON_CHANGE, target_change_hook, 0);
  check_preview->Click.connect(Bind<void>(&preview_change_hook, check_preview));
  
  /* default position */
  window->remap_window();
  window->center_window();

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "InvertColor");

  /* open the window */
  window->open_window_fg();

  if (window->get_killer() == button_ok)
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
  return false;
}

static void preview_change_hook(Widget* widget)
{
  set_config_bool("InvertColor", "Preview", widget->isSelected());
  make_preview();
}

static void make_preview()
{
  if (check_preview->isSelected())
    preview_restart(preview);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_invert_color_command()
{
  return new InvertColorCommand;
}
