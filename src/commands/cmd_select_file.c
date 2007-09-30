/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include <allegro/debug.h>
#include <allegro/unicode.h>

#include "jinete.h"

#include "core/app.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

bool command_enabled_select_file(const char *argument)
{
  /* with argument, the argument specified the ID of the GfxObj */
  if (argument) {
    int sprite_id = ustrtol(argument, NULL, 10);
    GfxObj *gfxobj = gfxobj_find(sprite_id);
    return gfxobj && gfxobj->type == GFXOBJ_SPRITE;
  }
  /* argument=NULL, means the select "Nothing" option  */
  else
    return TRUE;
}

bool command_checked_select_file(const char *argument)
{
  if (argument) {
    int sprite_id = ustrtol(argument, NULL, 10);
    GfxObj *gfxobj = gfxobj_find(sprite_id);
    return
      gfxobj && gfxobj->type == GFXOBJ_SPRITE &&
      current_sprite == (Sprite *)gfxobj;
  }
  else
    return current_sprite == NULL;
}

void command_execute_select_file(const char *argument)
{
  if (argument) {
    int sprite_id = ustrtol(argument, NULL, 10);
    GfxObj *gfxobj = gfxobj_find(sprite_id);
    ASSERT(gfxobj != NULL);

    sprite_show((Sprite *)gfxobj);
  }
  else {
    sprite_show(NULL);
  }
}
