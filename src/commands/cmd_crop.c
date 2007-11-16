/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "util/autocrop.h"
#include "util/crop.h"
#include "util/misc.h"

#endif

static bool command_enabled_crop(const char *argument);

/* ======================== */
/* crop_sprite              */
/* ======================== */

bool command_enabled_crop_sprite(const char *argument)
{
  return command_enabled_crop(argument);
}

void command_execute_crop_sprite(const char *argument)
{
  crop_sprite();
}

/* ======================== */
/* auto_crop_sprite         */
/* ======================== */

bool command_enabled_autocrop_sprite(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_autocrop_sprite(const char *argument)
{
  autocrop_sprite();
}

/* ======================== */
/* crop_layer               */
/* ======================== */

bool command_enabled_crop_layer(const char *argument)
{
  return command_enabled_crop(argument);
}

void command_execute_crop_layer(const char *argument)
{
  crop_layer();
}

/* ======================== */
/* crop_frame               */
/* ======================== */

bool command_enabled_crop_frame(const char *argument)
{
  return command_enabled_crop(argument);
}

void command_execute_crop_frame(const char *argument)
{
  crop_frame();
}

/**********************************************************************/
/* local */

static bool command_enabled_crop(const char *argument)
{
  if ((!current_sprite) ||
      (!current_sprite->layer) ||
      (!current_sprite->layer->readable) ||
      (!current_sprite->layer->writable) ||
      (!current_sprite->mask) ||
      (!current_sprite->mask->bitmap))
    return FALSE;
  else
    return GetImage() ? TRUE: FALSE;
}
