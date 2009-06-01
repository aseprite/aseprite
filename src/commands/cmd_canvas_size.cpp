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

#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undoable.h"
#include "widgets/colbar.h"

static bool cmd_canvas_size_enabled(const char *argument)
{
  CurrentSprite sprite;
  return sprite != NULL;
}

static void cmd_canvas_size_execute(const char *argument)
{
  JWidget window, left, top, right, bottom, ok;
  CurrentSprite sprite;

  // load the window widget
  window = load_widget("canvas.jid", "canvas_size");
  if (!window)
    return;

  if (!get_widgets(window,
		   "left", &left,
		   "top", &top,
		   "right", &right,
		   "bottom", &bottom,
		   "ok", &ok, NULL)) {
    jwidget_free(window);
    return;
  }

  jwindow_remap(window);
  jwindow_center(window);

  load_window_pos(window, "CanvasSize");
  jwidget_show(window);
  jwindow_open_fg(window);
  save_window_pos(window, "CanvasSize");

  if (jwindow_get_killer(window) == ok) {
    int x1 = -left->text_int();
    int y1 = -top->text_int();
    int x2 = sprite->w + right->text_int();
    int y2 = sprite->h + bottom->text_int();

    if (x2 <= x1) x2 = x1+1;
    if (y2 <= y1) y2 = y1+1;

    {
      Undoable undoable(sprite, "Canvas Size");
      int bgcolor = get_color_for_image(sprite->imgtype,
					colorbar_get_bg_color(app_get_colorbar()));
      undoable.crop_sprite(x1, y1, x2-x1, y2-y1, bgcolor);
      undoable.commit();
    }
    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);
  }

  jwidget_free(window);
}

Command cmd_canvas_size = {
  CMD_CANVAS_SIZE,
  cmd_canvas_size_enabled,
  NULL,
  cmd_canvas_size_execute,
  NULL
};
