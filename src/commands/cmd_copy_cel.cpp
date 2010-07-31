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

#include "jinete/jbase.h"

#include "commands/command.h"
#include "sprite_wrappers.h"
#include "dialogs/aniedit.h"
#include "util/celmove.h"

class CopyCelCommand : public Command
{
public:
  CopyCelCommand();
  Command* clone() const { return new CopyCelCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CopyCelCommand::CopyCelCommand()
  : Command("copy_cel",
	    "Copy Cel",
	    CmdUIOnlyFlag)
{
}

bool CopyCelCommand::onEnabled(Context* context)
{
  return animation_editor_is_movingcel();
}

void CopyCelCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  copy_cel(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_copy_cel_command()
{
  return new CopyCelCommand;
}
