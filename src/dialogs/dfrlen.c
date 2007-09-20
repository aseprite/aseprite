/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include <stdio.h>
#include <string.h>

#include "jinete/alert.h"
#include "jinete/widget.h"
#include "jinete/window.h"

#include "core/core.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/sprite.h"

#endif

/* if sprite_frpos < 0, set the frame length of all frames */
void dialogs_frame_length(int sprite_frpos)
{
  JWidget window, frpos, frlen, ok;
  char buf[64];

  if (!is_interactive() || !current_sprite)
    return;

  window = load_widget ("frlen.jid", "frame_duration");
  if (!window)
    return;

  if (!get_widgets(window,
		   "frpos", &frpos,
		   "frlen", &frlen,
		   "ok", &ok, NULL)) {
    jwidget_free(window);
    return;
  }

  if (sprite_frpos < 0)
    strcpy(buf, "All");
  else
    sprintf(buf, "%d", sprite_frpos);
  jwidget_set_text(frpos, buf);

  sprintf(buf, "%d", sprite_get_frlen(current_sprite, current_sprite->frpos));
  jwidget_set_text(frlen, buf);

  jwindow_open_fg(window);
  if (jwindow_get_killer(window) == ok) {
    int num = strtol(jwidget_get_text(frlen), NULL, 10);

    if (sprite_frpos < 0) {
      if (jalert("Warning"
		 "<<Do you want change the frame-rate of all frames?"
		 "<<(this operation can\'t be undoed)"
		 "||&Yes||&No") == 1)
	sprite_set_speed(current_sprite, num);
    }
    else
      sprite_set_frlen(current_sprite, num, sprite_frpos);
  }
  jwidget_free(window);
}
