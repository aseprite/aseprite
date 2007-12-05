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

#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/colbut.h"

#endif

/* TODO remove this */
void dialogs_frame_length(int sprite_frpos);

static bool cmd_sprite_properties_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_sprite_properties_execute(const char *argument)
{
  JWidget window, killer, name, type, size, frames, speed, ok;
  JWidget bgcolor_box, bgcolor_button;
  Sprite *sprite = current_sprite;
  char *imgtype_text;
  char buf[256];
  char *tmp;

  /* load the window widget */
  window = load_widget("sprprop.jid", "sprite_properties");
  if (!window)
    return;

  if (!get_widgets(window,
		   "name", &name,
		   "type", &type,
		   "size", &size,
		   "frames", &frames,
		   "bgcolor_box", &bgcolor_box,
		   "speed", &speed,
		   "ok", &ok, NULL)) {
    jwidget_free(window);
    return;
  }

  /* update widgets values */
  switch (sprite->imgtype) {
    case IMAGE_RGB:
      imgtype_text = "RGB";
      break;
    case IMAGE_GRAYSCALE:
      imgtype_text = "Grayscale";
      break;
    case IMAGE_INDEXED:
      imgtype_text = "Indexed";
      break;
    default:
      imgtype_text = "Unknown";
      break;
  }

  /* filename */
  jwidget_set_text(name, sprite->filename);
  jwidget_set_static_size(name, 256, 0);

  /* color mode */
  jwidget_set_text(type, imgtype_text);

  /* sprite size (width and height) */
  usprintf(buf, "%dx%d", sprite->w, sprite->h);
  jwidget_set_text(size, buf);

  /* how many frames */
  usprintf(buf, "%d", sprite->frames);
  jwidget_set_text(frames, buf);

  /* background color */
  tmp = color_from_image(sprite->imgtype, sprite->bgcolor);
  bgcolor_button = color_button_new(tmp, current_sprite->imgtype);
  jfree(tmp);

  jwidget_add_child(bgcolor_box, bgcolor_button);

  jwindow_remap(window);
  jwindow_center(window);

  for (;;) {
    load_window_pos(window, "SpriteProperties");
    jwindow_open_fg(window);
    save_window_pos(window, "SpriteProperties");

    killer = jwindow_get_killer(window);
    if (killer == ok) {
      int new_bgcolor;

      /* the background color changes */
      new_bgcolor = get_color_for_image(sprite->imgtype,
					color_button_get_color(bgcolor_button));

      /* set frames */
      if (sprite->bgcolor != new_bgcolor) {
	if (undo_is_enabled(sprite->undo))
	  undo_int(sprite->undo, (GfxObj *)sprite, &sprite->bgcolor);

	sprite_set_bgcolor(sprite, new_bgcolor);
      }

      /* update sprite in editors */
      update_screen_for_sprite(sprite);
      break;
    }
    else if (killer == speed) {
      dialogs_frame_length(-1);
    }
    else
      break;
  }

  jwidget_free(window);
}

Command cmd_sprite_properties = {
  CMD_SPRITE_PROPERTIES,
  cmd_sprite_properties_enabled,
  NULL,
  cmd_sprite_properties_execute,
  NULL
};
