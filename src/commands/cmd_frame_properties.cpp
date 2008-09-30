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

#include "jinete/jinete.h"

#include "commands/commands.h"
/* #include "core/app.h" */
#include "modules/gui.h"
#include "modules/sprites.h"
/* #include "raster/cel.h" */
/* #include "raster/layer.h" */
#include "raster/sprite.h"
/* #include "raster/undo.h" */

void dialogs_frame_length(int sprite_frame);

static bool cmd_frame_properties_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_frame_properties_execute(const char *argument)
{
  dialogs_frame_length(current_sprite->frame);
}

/* if sprite_frame < 0, set the frame length of all frames */
void dialogs_frame_length(int sprite_frame)
{
  JWidget window, frame, frlen, ok;
  char buf[64];

  window = load_widget("frlen.jid", "frame_duration");
  if (!window)
    return;

  if (!get_widgets(window,
		   "frame", &frame,
		   "frlen", &frlen,
		   "ok", &ok, NULL)) {
    jwidget_free(window);
    return;
  }

  if (sprite_frame < 0)
    strcpy(buf, "All");
  else
    sprintf(buf, "%d", sprite_frame+1);
  jwidget_set_text(frame, buf);

  sprintf(buf, "%d", sprite_get_frlen(current_sprite, current_sprite->frame));
  jwidget_set_text(frlen, buf);

  jwindow_open_fg(window);
  if (jwindow_get_killer(window) == ok) {
    int num = strtol(jwidget_get_text(frlen), NULL, 10);

    if (sprite_frame < 0) {
      if (jalert("Warning"
		 "<<Do you want change the frame-rate of all frames?"
		 "<<(this operation can\'t be undoed)"
		 "||&Yes||&No") == 1)
	sprite_set_speed(current_sprite, num);
    }
    else
      sprite_set_frlen(current_sprite, sprite_frame, num);
  }
  jwidget_free(window);
}

Command cmd_frame_properties = {
  CMD_FRAME_PROPERTIES,
  cmd_frame_properties_enabled,
  NULL,
  cmd_frame_properties_execute,
  NULL
};
