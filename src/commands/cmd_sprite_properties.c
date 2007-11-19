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

#include "jinete.h"

/* #include "core/app.h" */
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"

#endif

/* TODO remove this */
void dialogs_frame_length(int sprite_frpos);

bool command_enabled_sprite_properties(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_sprite_properties(const char *argument)
{
  JWidget window, killer, name, type, size, frames, speed, ok;
  Sprite *sprite = current_sprite;
  char *imgtype_text;
  char buf[256];

  /* load the window widget */
  window = load_widget("sprprop.jid", "sprite_properties");
  if (!window)
    return;

  if (!get_widgets(window,
		   "name", &name,
		   "type", &type,
		   "size", &size,
		   "frames", &frames,
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

  jwidget_set_text(name, sprite->filename);
  jwidget_set_text(type, imgtype_text);
  usprintf(buf, "%dx%d", sprite->w, sprite->h);
  jwidget_set_text(size, buf);
  usprintf(buf, "%d", sprite->frames);
  jwidget_set_text(frames, buf);

  jwindow_remap(window);
  jwindow_center(window);

  for (;;) {
    load_window_pos(window, "SpriteProperties");
    jwindow_open_fg(window);
    save_window_pos(window, "SpriteProperties");

    killer = jwindow_get_killer(window);
    if (killer == ok) {
      const char *filename = jwidget_get_text(name);
      int nframes = ustrtol(jwidget_get_text(frames), NULL, 10);

      nframes = MID(1, nframes, 9999);

      /* does filename change? */
      if (filename != sprite->filename) {
	sprite_set_filename(sprite, filename);
	rebuild_sprite_list();
      }

      sprite_set_frames(sprite, nframes);

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
