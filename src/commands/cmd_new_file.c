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

#include <assert.h>
#include <allegro/config.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/functions.h"
#include "util/misc.h"
#include "widgets/colbar.h"

static int _sprite_counter = 0;

/**
 * Shows the "New Sprite" dialog.
 */
static void cmd_new_file_execute(const char *argument)
{
  JWidget window, width, height, radio1, radio2, radio3, ok, bg_box;
  int imgtype, w, h, bg;
  char buf[1024];
  Sprite *sprite;
  color_t color;
  color_t bg_table[] = {
    color_mask(),
    color_rgb(0, 0, 0),
    color_rgb(255, 255, 255),
    color_rgb(255, 0, 255),
    colorbar_get_bg_color(app_get_colorbar())
  };

  /* load the window widget */
  window = load_widget("newspr.jid", "new_sprite");
  if (!window)
    return;

  width = jwidget_find_name(window, "width");
  height = jwidget_find_name(window, "height");
  radio1 = jwidget_find_name(window, "radio1");
  radio2 = jwidget_find_name(window, "radio2");
  radio3 = jwidget_find_name(window, "radio3");
  ok = jwidget_find_name(window, "ok_button");
  bg_box = jwidget_find_name(window, "bg_box");

  /* default values: Indexed, 320x200, Transparent */
  imgtype = get_config_int("NewSprite", "Type", IMAGE_RGB);
  imgtype = MID(IMAGE_RGB, imgtype, IMAGE_INDEXED);
  w = get_config_int("NewSprite", "Width", 320); /* default = 320 */
  h = get_config_int("NewSprite", "Height", 200); /* default = 200 */
  bg = get_config_int("NewSprite", "Background", 2); /* default = white */

  usprintf(buf, "%d", w); jwidget_set_text(width, buf);
  usprintf(buf, "%d", h); jwidget_set_text(height, buf);

  /* select image-type */
  switch (imgtype) {
    case IMAGE_RGB:       jwidget_select(radio1); break;
    case IMAGE_GRAYSCALE: jwidget_select(radio2); break;
    case IMAGE_INDEXED:   jwidget_select(radio3); break;
  }

  /* select background color */
  jlistbox_select_index(bg_box, bg);

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == ok) {
    bool ok = FALSE;

    /* get the options */
    if (jwidget_is_selected(radio1))      imgtype = IMAGE_RGB;
    else if (jwidget_is_selected(radio2)) imgtype = IMAGE_GRAYSCALE;
    else if (jwidget_is_selected(radio3)) imgtype = IMAGE_INDEXED;

    w = ustrtol(jwidget_get_text(width), NULL, 10);
    h = ustrtol(jwidget_get_text(height), NULL, 10);
    bg = jlistbox_get_selected_index(bg_box);

    w = MID(1, w, 9999);
    h = MID(1, h, 9999);

    /* select the color */
    color = color_mask();

    if (bg >= 0 && bg <= 4) {
      color = bg_table[bg];
      ok = TRUE;
    }

    if (ok) {
      /* save the configuration */
      set_config_int("NewSprite", "Type", imgtype);
      set_config_int("NewSprite", "Width", w);
      set_config_int("NewSprite", "Height", h);
      set_config_int("NewSprite", "Background", bg);

      /* create the new sprite */
      sprite = NewSprite(imgtype, w, h);
      if (!sprite) {
	console_printf("Not enough memory to allocate the sprite\n");
      }
      else {
	usprintf(buf, "Sprite-%04d", ++_sprite_counter);
	sprite_set_filename(sprite, buf);

	/* if the background color isn't transparent, we have to
	   convert the `Layer 1' in a `Background' */
	if (color_type(color) != COLOR_TYPE_MASK) {
	  BackgroundFromLayer(sprite);

	  /* clear the image to */
	  image_clear(GetImage(sprite),
		      get_color_for_image(imgtype, color));
	}

	/* the undo should be disabled because we use NewSprite to
	   create it (a function for scripts) */
	assert(undo_is_disabled(sprite->undo));

	/* enable undo */
	undo_enable(sprite->undo);

	/* show the sprite to the user */
	sprite_show(sprite);
      }
    }
  }

  jwidget_free(window);
}

Command cmd_new_file = {
  CMD_NEW_FILE,
  NULL,
  NULL,
  cmd_new_file_execute,
  NULL
};
