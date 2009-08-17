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

#include <allegro.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/commands.h"
#include "core/app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "raster/sprite.h"

static bool cmd_duplicate_sprite_enabled(const char *argument)
{
  const CurrentSpriteReader sprite;
  return sprite.is_valid();
}

static void cmd_duplicate_sprite_execute(const char *argument)
{
  JWidget src_name, dst_name, flatten;
  const CurrentSpriteReader sprite;
  char buf[1024];

  /* load the window widget */
  JWidgetPtr window(load_widget("dupspr.jid", "duplicate_sprite"));

  src_name = jwidget_find_name(window, "src_name");
  dst_name = jwidget_find_name(window, "dst_name");
  flatten = jwidget_find_name(window, "flatten");

  jwidget_set_text(src_name, get_filename(sprite->filename));

  sprintf(buf, "%s %s", sprite->filename, _("Copy"));
  jwidget_set_text(dst_name, buf);

  if (get_config_bool("DuplicateSprite", "Flatten", FALSE))
    jwidget_select(flatten);

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    set_config_bool("DuplicateSprite", "Flatten",
		    jwidget_is_selected(flatten));

    // make a copy of the current sprite
    Sprite *sprite_copy;
    if (jwidget_is_selected(flatten))
      sprite_copy = sprite_new_flatten_copy(sprite);
    else
      sprite_copy = sprite_new_copy(sprite);

    if (sprite_copy != NULL) {
      sprite_set_filename(sprite_copy, jwidget_get_text(dst_name));

      UIContext* context = UIContext::instance();

      context->add_sprite(sprite_copy);
      context->set_current_sprite(sprite_copy);
      context->show_sprite(sprite_copy);
    }
  }
}

Command cmd_duplicate_sprite = {
  CMD_DUPLICATE_SPRITE,
  cmd_duplicate_sprite_enabled,
  NULL,
  cmd_duplicate_sprite_execute,
};
