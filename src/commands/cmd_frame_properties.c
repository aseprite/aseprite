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

#include "jinete.h"

#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"

#endif

bool command_enabled_cel_properties(const char *argument)
{
  return current_sprite != NULL;
}

void command_execute_cel_properties(const char *argument)
{
  JWidget window, entry_frpos, entry_xpos, entry_ypos, slider_opacity, button_ok;
  Sprite *sprite;
  Layer *layer;
  Cel *cel;
  char buf[1024];

  /* get current sprite */
  sprite = current_sprite;
  if (!sprite)
    return;

  /* get selected layer */
  layer = sprite->layer;
  if (!layer)
    return;

  /* get current cel */
  cel = layer_get_cel(layer, sprite->frpos);
  if (!cel)
    return;

  window = load_widget("frmprop.jid", "frame_properties");
  if (!window)
    return;

  entry_frpos = jwidget_find_name(window, "frpos");
  entry_xpos = jwidget_find_name(window, "xpos");
  entry_ypos = jwidget_find_name(window, "ypos");
  slider_opacity = jwidget_find_name(window, "opacity");
  button_ok = jwidget_find_name(window, "ok");

  sprintf(buf, "%d", cel->frpos); jwidget_set_text(entry_frpos, buf);
  sprintf(buf, "%d", cel->x); jwidget_set_text(entry_xpos, buf);
  sprintf(buf, "%d", cel->y); jwidget_set_text(entry_ypos, buf);
  jslider_set_value(slider_opacity, cel->opacity);

  while (TRUE) {
    jwindow_open_fg(window);

    if (jwindow_get_killer(window) == button_ok) {
      int new_frpos, new_xpos, new_ypos;
      Cel *existent_cel;

      new_frpos = strtol(jwidget_get_text(entry_frpos), NULL, 10);
      new_frpos = MID(0, new_frpos, sprite->frames-1);
      existent_cel = layer_get_cel(layer, new_frpos);
	
      if (new_frpos != cel->frpos && existent_cel) {
	jalert(_("Error"
		 "<<You can't change frame position to %d."
		 "<<Already there is a cel in that position."
		 "||&OK"), new_frpos);
      }
      else {
	/* WE MUST REMOVE THE FRAME BEFORE CALL frame_set_frpos() */
	if (undo_is_enabled(sprite->undo)) {
	  undo_open(sprite->undo);
	  undo_remove_cel(sprite->undo, layer, cel);
	}

	layer_remove_cel(layer, cel);

	/* change frame properties */
	new_xpos = strtol(jwidget_get_text(entry_xpos), NULL, 10);
	new_ypos = strtol(jwidget_get_text(entry_ypos), NULL, 10);

	cel_set_frpos(cel, new_frpos);
	cel_set_position(cel,
			 MID(-9999, new_xpos, 9999),
			 MID(-9999, new_ypos, 9999));
	cel_set_opacity(cel, jslider_get_value(slider_opacity));

	/* add again the same frame */
	if (undo_is_enabled(sprite->undo)) {
	  undo_add_cel(sprite->undo, layer, cel);
	  undo_close(sprite->undo);
	}

	layer_add_cel(layer, cel);

	/* set the sprite position, refresh and break the loop */
	sprite_set_frpos(sprite, new_frpos);
	GUI_Refresh(sprite);
	break;
      }
    }
    else
      break;
  }

  jwidget_free(window);
}
