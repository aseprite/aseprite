/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "util/autocrop.h"
#include "util/crop.h"
#include "util/misc.h"

static bool cmd_crop_enabled(const char *argument);

/* ======================== */
/* crop_sprite              */
/* ======================== */

static bool cmd_crop_sprite_enabled(const char *argument)
{
  return cmd_crop_enabled(argument);
}

static void cmd_crop_sprite_execute(const char *argument)
{
  crop_sprite();
}

/* ======================== */
/* autocrop_sprite          */
/* ======================== */

static bool cmd_autocrop_sprite_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_autocrop_sprite_execute(const char *argument)
{
  autocrop_sprite();
}

/* ======================== */
/* crop_layer               */
/* ======================== */

static bool cmd_crop_layer_enabled(const char *argument)
{
  return cmd_crop_enabled(argument);
}

static void cmd_crop_layer_execute(const char *argument)
{
  crop_layer();
}

/* ======================== */
/* crop_cel               */
/* ======================== */

static bool cmd_crop_cel_enabled(const char *argument)
{
  return cmd_crop_enabled(argument);
}

static void cmd_crop_cel_execute(const char *argument)
{
  crop_cel();
}

/**********************************************************************/
/* local */

static bool cmd_crop_enabled(const char *argument)
{
  if ((!current_sprite) ||
      (!current_sprite->layer) ||
      (!current_sprite->layer->readable) ||
      (!current_sprite->layer->writable) ||
      (!current_sprite->mask) ||
      (!current_sprite->mask->bitmap))
    return FALSE;
  else
    return GetImage(current_sprite) ? TRUE: FALSE;
}

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

Command cmd_crop_layer = {
  CMD_CROP_LAYER,
  cmd_crop_layer_enabled,
  NULL,
  cmd_crop_layer_execute,
  NULL
};

Command cmd_crop_cel = {
  CMD_CROP_CEL,
  cmd_crop_cel_enabled,
  NULL,
  cmd_crop_cel_execute,
  NULL
};
