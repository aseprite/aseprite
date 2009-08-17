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

#include <assert.h>
#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console.h"
#include "core/color.h"
#include "core/app.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "widgets/statebar.h"

#include <stdexcept>

static bool cmd_new_frame_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return
    sprite != NULL &&
    sprite->layer != NULL &&
    layer_is_readable(sprite->layer) &&
    layer_is_writable(sprite->layer) &&
    layer_is_image(sprite->layer);
}

static void cmd_new_frame_execute(const char *argument)
{
  CurrentSpriteWriter sprite;
  {
    Undoable undoable(sprite, "New Frame");
    undoable.new_frame();
    undoable.commit();
  }
  update_screen_for_sprite(sprite);
  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("New frame %d/%d"),
		     sprite->frame+1,
		     sprite->frames);
}

Command cmd_new_frame = {
  CMD_NEW_FRAME,
  cmd_new_frame_enabled,
  NULL,
  cmd_new_frame_execute,
};
