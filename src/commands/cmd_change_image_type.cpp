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

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/quant.h"
#include "raster/sprite.h"
#include "undoable.h"

static bool cmd_change_image_type_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL &&
    ((ustrcmp(argument, "rgb") == 0 && sprite->imgtype != IMAGE_RGB) ||
     (ustrcmp(argument, "grayscale") == 0 && sprite->imgtype != IMAGE_GRAYSCALE) ||
     (ustrncmp(argument, "indexed", 7) == 0 && sprite->imgtype != IMAGE_INDEXED));
}

static void cmd_change_image_type_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  int destimgtype = sprite->imgtype;
  int dithermethod = DITHERING_NONE;

  if (ustrcmp(argument, "rgb") == 0) {
    destimgtype = IMAGE_RGB;
  }
  else if (ustrcmp(argument, "grayscale") == 0) {
    destimgtype = IMAGE_GRAYSCALE;
  }
  else if (ustrcmp(argument, "indexed") == 0) {
    destimgtype = IMAGE_INDEXED;
  }
  else if (ustrcmp(argument, "indexed-dither") == 0) {
    destimgtype = IMAGE_INDEXED;
    dithermethod = DITHERING_ORDERED;
  }

  {
    CurrentSpriteRgbMap rgbmap;
    Undoable undoable(sprite, "Color Mode Change");
    undoable.set_imgtype(destimgtype, dithermethod);
    undoable.commit();
  }

  app_refresh_screen(sprite);
}

Command cmd_change_image_type = {
  CMD_CHANGE_IMAGE_TYPE,
  cmd_change_image_type_enabled,
  NULL,
  cmd_change_image_type_execute,
};
