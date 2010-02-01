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

#include <string>

#include "commands/command.h"
#include "commands/params.h"
#include "app.h"
#include "widgets/colbar.h"

class ChangeColorCommand : public Command
{
  enum Change {
    None,
    IncrementIndex,
    DecrementIndex,
  };

  /**
   * True means "change background color", false the foreground color.
   */
  bool m_background;

  Change m_change;

public:
  ChangeColorCommand();

protected:
  void load_params(Params* params);
  void execute(Context* context);
};

ChangeColorCommand::ChangeColorCommand()
  : Command("change_color",
	    "Change Color",
	    CmdUIOnlyFlag)
{
  m_background = false;
  m_change = None;
}

void ChangeColorCommand::load_params(Params* params)
{
  std::string target = params->get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;

  std::string change = params->get("change");
  if (change == "increment-index") m_change = IncrementIndex;
  else if (change == "decrement-index") m_change = DecrementIndex;
}

void ChangeColorCommand::execute(Context* context)
{
  JWidget colorbar = app_get_colorbar();
  color_t color = m_background ? colorbar_get_bg_color(colorbar):
				 colorbar_get_fg_color(colorbar);
  int imgtype = app_get_current_image_type();

  switch (m_change) {
    case None:
      // do nothing
      break;
    case IncrementIndex:
      if (color_type(color) == COLOR_TYPE_INDEX) {
	int index = color_get_index(imgtype, color);
	if (index < 255)	// TODO use sprite palette limit
	  color = color_index(index+1);
      }
      else
	color = color_index(0);
      break;
    case DecrementIndex:
      if (color_type(color) == COLOR_TYPE_INDEX) {
	int index = color_get_index(imgtype, color);
	if (index > 0)
	  color = color_index(index-1);
      }
      else
	color = color_index(0);
      break;
  }

  if (m_background)
    colorbar_set_bg_color(colorbar, color);
  else
    colorbar_set_fg_color(colorbar, color);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_change_color_command()
{
  return new ChangeColorCommand;
}
