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

#include "commands/command.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/color_bar.h"
#include "util/autocrop.h"
#include "util/misc.h"
#include "sprite_wrappers.h"
#include "app/color_utils.h"

//////////////////////////////////////////////////////////////////////
// crop_sprite

class CropSpriteCommand : public Command
{
public:
  CropSpriteCommand();
  Command* clone() const { return new CropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CropSpriteCommand::CropSpriteCommand()
  : Command("crop_sprite",
	    "Crop Sprite",
	    CmdRecordableFlag)
{
}

bool CropSpriteCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL &&
    sprite->getMask() != NULL &&
    sprite->getMask()->bitmap != NULL;
}

void CropSpriteCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    Undoable undoable(sprite, "Sprite Crop");
    int bgcolor = color_utils::color_for_image(app_get_colorbar()->getBgColor(), sprite->getImgType());

    undoable.crop_sprite(sprite->getMask()->x,
			 sprite->getMask()->y,
			 sprite->getMask()->w,
			 sprite->getMask()->h,
			 bgcolor);
    undoable.commit();
  }
  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// autocrop_sprite

class AutocropSpriteCommand : public Command
{
public:
  AutocropSpriteCommand();
  Command* clone() const { return new AutocropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

AutocropSpriteCommand::AutocropSpriteCommand()
  : Command("autocrop_sprite",
	    "Autocrop Sprite",
	    CmdRecordableFlag)
{
}

bool AutocropSpriteCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void AutocropSpriteCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    int bgcolor = color_utils::color_for_image(app_get_colorbar()->getBgColor(), sprite->getImgType());

    Undoable undoable(sprite, "Sprite Autocrop");
    undoable.autocrop_sprite(bgcolor);
    undoable.commit();
  }
  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_crop_sprite_command()
{
  return new CropSpriteCommand;
}

Command* CommandFactory::create_autocrop_sprite_command()
{
  return new AutocropSpriteCommand;
}
