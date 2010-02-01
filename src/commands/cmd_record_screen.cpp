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

#include <allegro/gfx.h>

#include "jinete/jbase.h"
#include "jinete/jalert.h"

#include "commands/command.h"
#include "util/recscr.h"

//////////////////////////////////////////////////////////////////////
// record_screen

class RecordScreenCommand : public Command
{
public:
  RecordScreenCommand();
  Command* clone() { return new RecordScreenCommand(*this); }

protected:
  bool checked(Context* context);
  void execute(Context* context);
};

RecordScreenCommand::RecordScreenCommand()
  : Command("record_screen",
	    "Record Screen",
	    CmdUIOnlyFlag)
{
}

bool RecordScreenCommand::checked(Context* context)
{
  return is_rec_screen();
}

void RecordScreenCommand::execute(Context* context)
{
  if (is_rec_screen())
    rec_screen_off();
  else if (bitmap_color_depth(screen) == 8
	   || jalert(_("Warning"
		       "<<The display isn't in a 8 bpp resolution, the recording"
		       "<<process can be really slow. It's recommended to use a"
		       "<<8 bpp to make it faster."
		       "<<Do you want to continue anyway?"
		       "||Yes||No")) == 1) {
    rec_screen_on();
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_record_screen_command()
{
  return new RecordScreenCommand;
}
