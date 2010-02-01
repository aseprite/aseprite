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
#include "commands/commands.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// invert_mask

class InvertMaskCommand : public Command
{
public:
  InvertMaskCommand();
  Command* clone() { return new InvertMaskCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

InvertMaskCommand::InvertMaskCommand()
  : Command("invert_mask",
	    "Invert Mask",
	    CmdRecordableFlag)
{
}

bool InvertMaskCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void InvertMaskCommand::execute(Context* context)
{
  bool has_mask = false;
  {
    const CurrentSpriteReader sprite(context);
    if (sprite->mask->bitmap)
      has_mask = true;
  }

  // without mask?...
  if (!has_mask) {
    // so we select all
    Command* mask_all_cmd =
      CommandsModule::instance()->get_command_by_name(CommandId::mask_all);
    context->execute_command(mask_all_cmd);
  }
  // invert the current mask
  else {
    CurrentSpriteWriter sprite(context);

    /* undo */
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Mask Invert");
      undo_set_mask(sprite->undo, sprite);
    }

    /* create a new mask */
    Mask* mask = mask_new();

    /* select all the sprite area */
    mask_replace(mask, 0, 0, sprite->w, sprite->h);

    /* remove in the new mask the current sprite marked region */
    image_rectfill(mask->bitmap,
		   sprite->mask->x, sprite->mask->y,
		   sprite->mask->x + sprite->mask->w-1,
		   sprite->mask->y + sprite->mask->h-1, 0);

    /* invert the current mask in the sprite */
    mask_invert(sprite->mask);
    if (sprite->mask->bitmap) {
      /* copy the inverted region in the new mask */
      image_copy(mask->bitmap, sprite->mask->bitmap,
		 sprite->mask->x, sprite->mask->y);
    }

    /* we need only need the area inside the sprite */
    mask_intersect(mask, 0, 0, sprite->w, sprite->h);

    /* set the new mask */
    sprite_set_mask(sprite, mask);
    mask_free(mask);

    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_invert_mask_command()
{
  return new InvertMaskCommand;
}
