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

#include "commands/command.h"
#include "gui/base.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "util/clipboard.h"
#include "util/misc.h"

class CutCommand : public Command
{
public:
  CutCommand();
  Command* clone() const { return new CutCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CutCommand::CutCommand()
  : Command("Cut",
	    "Cut",
	    CmdUIOnlyFlag)
{
}

bool CutCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  if ((sprite) &&
      (sprite->getCurrentLayer()) &&
      (sprite->getCurrentLayer()->is_readable()) &&
      (sprite->getCurrentLayer()->is_writable()) &&
      (sprite->getMask()) &&
      (sprite->getMask()->bitmap))
    return sprite->getCurrentImage() ? true: false;
  else
    return false;
}

void CutCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  clipboard::cut(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCutCommand()
{
  return new CutCommand;
}
