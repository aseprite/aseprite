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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/fx/effectbg.h"
#include "console.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
#include "effect/colcurve.h"
#include "effect/effect.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/curvedit.h"
#include "widgets/preview.h"
#include "widgets/target.h"

static Curve* the_curve = NULL;
static JWidget check_preview, preview;

static bool window_msg_proc(JWidget widget, JMessage msg);
static void make_preview();

// Slot for App::Exit signal 
static void on_exit_delete_curve()
{
  curve_free(the_curve);
}

//////////////////////////////////////////////////////////////////////
// ColorCurveCommand

class ColorCurveCommand : public Command
{
public:
  ColorCurveCommand();
  Command* clone() const { return new ColorCurveCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

ColorCurveCommand::ColorCurveCommand()
  : Command("color_curve",
	    "Color Curve",
	    CmdRecordableFlag)
{
}

bool ColorCurveCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void ColorCurveCommand::execute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  JWidget button_ok;
  JWidget view_curve, curve_editor;
  JWidget box_target, target_button;

  if (!the_curve) {
    /* default curve */
    the_curve = curve_new(CURVE_LINEAR);
    curve_add_point(the_curve, curve_point_new(0, 0));
    curve_add_point(the_curve, curve_point_new(255, 255));

    App::instance()->Exit.connect(&on_exit_delete_curve);
  }

  FramePtr window(load_widget("color_curve.xml", "color_curve"));
  get_widgets(window,
	      "preview", &check_preview,
	      "button_ok", &button_ok,
	      "curve", &view_curve,
	      "target", &box_target, NULL);

  Effect effect(sprite, "color_curve");
  effect_set_target(&effect, TARGET_RED_CHANNEL | TARGET_GREEN_CHANNEL |
			     TARGET_BLUE_CHANNEL | TARGET_ALPHA_CHANNEL);

  preview = preview_new(&effect);

  set_color_curve(the_curve);

  curve_editor = curve_editor_new(the_curve, 0, 0, 255, 255);
  target_button = target_button_new(sprite->getImgType(), true);
  target_button_set_target(target_button, effect.target);

  if (get_config_bool("ColorCurve", "Preview", true))
    jwidget_select(check_preview);

  jview_attach(view_curve, curve_editor);
  jwidget_set_min_size(view_curve, 128, 64);

  jwidget_add_child(box_target, target_button);
  jwidget_add_child(window, preview);

  jwidget_add_hook(window, -1, window_msg_proc, NULL);
  
  /* default position */
  window->remap_window();
  window->center_window();

  /* first preview */
  make_preview();

  /* load window configuration */
  load_window_pos(window, "ColorCurve");

  /* open the window */
  window->open_window_fg();

  if (window->get_killer() == button_ok)
    effect_apply_to_target_with_progressbar(&effect);

  /* update editors */
  update_screen_for_sprite(sprite);

  /* save window configuration */
  save_window_pos(window, "ColorCurve");
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
  return false;
}

static void make_preview()
{
  if (jwidget_is_selected(check_preview))
    preview_restart(preview);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_color_curve_command()
{
  return new ColorCurveCommand;
}
