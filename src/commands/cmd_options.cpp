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

#include <allegro.h>

#include "jinete/jinete.h"

#include "commands/command.h"
#include "core/cfg.h"
#include "modules/editors.h"
#include "modules/gui.h"

static JWidget slider_x, slider_y, check_lockmouse;

static bool slider_mouse_hook(JWidget widget, void *data);

//////////////////////////////////////////////////////////////////////
// options

class OptionsCommand : public Command
{
public:
  OptionsCommand();
  Command* clone() { return new OptionsCommand(*this); }

protected:
  void execute(Context* context);
};

OptionsCommand::OptionsCommand()
  : Command("options",
	    "Options",
	    CmdUIOnlyFlag)
{
}

void OptionsCommand::execute(Context* context)
{
  JWidget check_smooth;
  JWidget button_ok;
  JWidget move_click2, draw_click2, killer;
  JWidget undo_size_limit;

  /* load the window widget */
  FramePtr window(load_widget("options.xml", "options"));
  get_widgets(window,
	      "smooth", &check_smooth,
	      "move_click2", &move_click2,
	      "draw_click2", &draw_click2,
	      "undo_size_limit", &undo_size_limit,
	      "button_ok", &button_ok, NULL);

  if (get_config_bool("Options", "MoveClick2", false))
    jwidget_select(move_click2);
  if (get_config_bool("Options", "DrawClick2", false))
    jwidget_select(draw_click2);

  if (get_config_bool("Options", "MoveSmooth", true))
    jwidget_select(check_smooth);

  undo_size_limit->setTextf("%d", get_config_int("Options", "UndoSizeLimit", 8));

  window->open_window_fg();
  killer = window->get_killer();

  if (killer == button_ok) {
    int undo_size_limit_value;
    
    set_config_bool("Options", "MoveSmooth", jwidget_is_selected(check_smooth));
    set_config_bool("Options", "MoveClick2", jwidget_is_selected(move_click2));
    set_config_bool("Options", "DrawClick2", jwidget_is_selected(draw_click2));

    undo_size_limit_value = undo_size_limit->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);

    /* save configuration */
    flush_config_file();
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_options_command()
{
  return new OptionsCommand;
}
