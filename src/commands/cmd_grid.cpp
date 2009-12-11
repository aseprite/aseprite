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

#include <allegro/unicode.h>

#include "commands/command.h"
#include "app.h"
#include "modules/editors.h"
#include "modules/tools.h"
#include "widgets/statebar.h"

//////////////////////////////////////////////////////////////////////
// show_grid

class ShowGridCommand : public Command
{
public:
  ShowGridCommand()
    : Command("show_grid",
	      "Show Grid",
	      CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new ShowGridCommand(*this); }

protected:
  bool checked(Context* context)
  {
    return get_view_grid();
  }
  
  void execute(Context* context)
  {
    set_view_grid(get_view_grid() ? false: true);
    refresh_all_editors();
  }
};

//////////////////////////////////////////////////////////////////////
// snap_to_grid

class SnapToGridCommand : public Command
{
public:
  SnapToGridCommand()
    : Command("snap_to_grid",
	      "Snap to Grid",
	      CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new SnapToGridCommand(*this); }

protected:
  bool checked(Context* context)
  {
    return get_use_grid();
  }
  
  void execute(Context* context)
  {
    char buf[512];

    set_use_grid(get_use_grid() ? false: true);
    refresh_all_editors();

    usprintf(buf, _("Snap to grid: %s"),
	     get_use_grid() ? _("On"):
			      _("Off"));

    statusbar_set_text(app_get_statusbar(), 250, buf);
  }
};

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_show_grid_command()
{
  return new ShowGridCommand;
}

Command* CommandFactory::create_snap_to_grid_command()
{
  return new SnapToGridCommand;
}
