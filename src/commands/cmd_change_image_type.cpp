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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "core/cfg.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/quant.h"
#include "raster/sprite.h"

static bool cmd_change_image_type_enabled(const char *argument)
{
  CurrentSprite sprite;
  return sprite;
}

static void cmd_change_image_type_execute(const char *argument)
{
  JWidget window, from, radio1, radio2, radio3, dither1, dither2;
  CurrentSprite current_sprite;

  /* load the window widget */
  window = load_widget("imgtype.jid", "image_type");
  if (!window)
    return;

  from = jwidget_find_name(window, "from");
  radio1 = jwidget_find_name(window, "imgtype1");
  radio2 = jwidget_find_name(window, "imgtype2");
  radio3 = jwidget_find_name(window, "imgtype3");
  dither1 = jwidget_find_name(window, "dither1");
  dither2 = jwidget_find_name(window, "dither2");

  if (current_sprite->imgtype == IMAGE_RGB) {
    jwidget_set_text(from, _("RGB"));
    jwidget_disable(radio1);
    jwidget_select(radio3);	/* to Indexed by default */
  }
  else if (current_sprite->imgtype == IMAGE_GRAYSCALE) {
    jwidget_set_text(from, _("Grayscale"));
    jwidget_disable(radio2);
    jwidget_select(radio1);	/* to RGB by default */
  }
  else if (current_sprite->imgtype == IMAGE_INDEXED) {
    jwidget_set_text(from, _("Indexed"));
    jwidget_disable(radio3);
    jwidget_select(radio1);	/* to RGB by default */
  }

  if (get_config_bool("Options", "Dither", FALSE))
    jwidget_select(dither2);
  else
    jwidget_select(dither1);

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    int destimgtype, dithermethod;

    if (jwidget_is_selected(radio1))
      destimgtype = IMAGE_RGB;
    else if (jwidget_is_selected(radio2))
      destimgtype = IMAGE_GRAYSCALE;
    else
      destimgtype = IMAGE_INDEXED;

    if (jwidget_is_selected(dither1))
      dithermethod = DITHERING_NONE;
    else
      dithermethod = DITHERING_ORDERED;

    {
      CurrentSpriteRgbMap rgbmap;
      sprite_set_imgtype(current_sprite, destimgtype, dithermethod);
    }

    app_refresh_screen();
  }

  jwidget_free(window);
}

Command cmd_change_image_type = {
  CMD_CHANGE_IMAGE_TYPE,
  cmd_change_image_type_enabled,
  NULL,
  cmd_change_image_type_execute,
  NULL
};
