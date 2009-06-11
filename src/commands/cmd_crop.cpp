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

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/colbar.h"
#include "util/autocrop.h"
#include "util/misc.h"

/* ======================== */
/* crop_sprite              */
/* ======================== */

static bool cmd_crop_sprite_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL &&
    sprite->mask != NULL &&
    sprite->mask->bitmap != NULL;
}

static void cmd_crop_sprite_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  {
    Undoable undoable(sprite, "Sprite Crop");
    int bgcolor = get_color_for_image(sprite->imgtype,
				      colorbar_get_bg_color(app_get_colorbar()));
    undoable.crop_sprite(sprite->mask->x,
			 sprite->mask->y,
			 sprite->mask->w,
			 sprite->mask->h,
			 bgcolor);
    undoable.commit();
  }
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

/* ======================== */
/* autocrop_sprite          */
/* ======================== */

static bool cmd_autocrop_sprite_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return sprite != NULL;
}

static void cmd_autocrop_sprite_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  {
    Undoable undoable(sprite, "Sprite Autocrop");
    undoable.autocrop_sprite(colorbar_get_bg_color(app_get_colorbar()));
    undoable.commit();
  }
  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

/**********************************************************************/
/* local */

Command cmd_crop_sprite = {
  CMD_CROP_SPRITE,
  cmd_crop_sprite_enabled,
  NULL,
  cmd_crop_sprite_execute,
  NULL
};

Command cmd_autocrop_sprite = {
  CMD_AUTOCROP_SPRITE,
  cmd_autocrop_sprite_enabled,
  NULL,
  cmd_autocrop_sprite_execute,
  NULL
};
