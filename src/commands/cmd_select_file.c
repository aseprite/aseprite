/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007,
 *               2008  David A. Capello
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

#include <assert.h>
#include <allegro/debug.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

static bool cmd_select_file_enabled(const char *argument)
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

static bool cmd_select_file_checked(const char *argument)
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

static void cmd_select_file_execute(const char *argument)
{
  if (argument) {
    int sprite_id = ustrtol(argument, NULL, 10);
    GfxObj *gfxobj = gfxobj_find(sprite_id);
    assert(gfxobj != NULL);

    sprite_show((Sprite *)gfxobj);
  }
  else {
    sprite_show(NULL);
  }
}

Command cmd_select_file = {
  CMD_SELECT_FILE,
  cmd_select_file_enabled,
  cmd_select_file_checked,
  cmd_select_file_execute,
  NULL
};
