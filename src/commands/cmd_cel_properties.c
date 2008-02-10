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

#ifndef USE_PRECOMPILED_HEADER

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"

#endif

static bool cmd_cel_properties_enabled(const char *argument)
{
  return
    is_current_sprite_not_locked()
    && current_sprite->layer
    && layer_get_cel(current_sprite->layer,
		     current_sprite->frame) != NULL;
}

static void cmd_cel_properties_execute(const char *argument)
{
  JWidget window = NULL;
  JWidget entry_frame, entry_xpos, entry_ypos, slider_opacity, button_ok;
  Sprite *sprite;
  Layer *layer;
  Cel *cel;
  char buf[1024];

  /* get current sprite */
  sprite = lock_current_sprite();
  if (!sprite)
    return;

  /* get selected layer */
  layer = sprite->layer;
  if (!layer)
    goto done;

  /* get current cel */
  cel = layer_get_cel(layer, sprite->frame);
  if (!cel)
    goto done;

  window = load_widget("celprop.jid", "cel_properties");
  if (!window)
    goto done;

  entry_frame = jwidget_find_name(window, "frame");
  entry_xpos = jwidget_find_name(window, "xpos");
  entry_ypos = jwidget_find_name(window, "ypos");
  slider_opacity = jwidget_find_name(window, "opacity");
  button_ok = jwidget_find_name(window, "ok");

  /* if the layer isn't writable */
  if (!layer_is_writable(layer)) {
    jwidget_set_text(button_ok, _("Locked"));
    jwidget_disable(button_ok);
  }

  sprintf(buf, "%d", cel->frame+1);
  jwidget_set_text(entry_frame, buf);

  sprintf(buf, "%d", cel->x);
  jwidget_set_text(entry_xpos, buf);

  sprintf(buf, "%d", cel->y);
  jwidget_set_text(entry_ypos, buf);

  jslider_set_value(slider_opacity, cel->opacity);

  while (TRUE) {
    jwindow_open_fg(window);

    if (jwindow_get_killer(window) == button_ok) {
      int new_frame, new_xpos, new_ypos;
      Cel *existent_cel;

      new_frame = strtol(jwidget_get_text(entry_frame), NULL, 10);
      new_frame = MID(0, new_frame-1, sprite->frames-1);
      existent_cel = layer_get_cel(layer, new_frame);

      if (new_frame != cel->frame && existent_cel) {
	jalert(_("Error"
		 "<<You can't move the cel to frame %d."
		 "<<There is already a cel in that position."
		 "||&OK"), new_frame+1);
      }
      else {
	/* WE MUST REMOVE THE FRAME BEFORE CALL cel_set_frame() */
	if (undo_is_enabled(sprite->undo)) {
	  undo_open(sprite->undo);
	  undo_remove_cel(sprite->undo, layer, cel);
	}

	layer_remove_cel(layer, cel);

	/* change cel properties */
	new_xpos = strtol(jwidget_get_text(entry_xpos), NULL, 10);
	new_ypos = strtol(jwidget_get_text(entry_ypos), NULL, 10);

	cel_set_frame(cel, new_frame);
	cel_set_position(cel,
			 MID(-9999, new_xpos, 9999),
			 MID(-9999, new_ypos, 9999));
	cel_set_opacity(cel, jslider_get_value(slider_opacity));

	/* add again the same cel */
	if (undo_is_enabled(sprite->undo)) {
	  undo_add_cel(sprite->undo, layer, cel);
	  undo_close(sprite->undo);
	}

	layer_add_cel(layer, cel);

	/* set the sprite position, refresh and break the loop */
	sprite_set_frame(sprite, new_frame);
	update_screen_for_sprite(sprite);
	break;
      }
    }
    else
      break;
  }

done:;
  if (window)
    jwidget_free(window);

  sprite_unlock(sprite);
}

Command cmd_cel_properties = {
  CMD_CEL_PROPERTIES,
  cmd_cel_properties_enabled,
  NULL,
  cmd_cel_properties_execute,
  NULL
};
