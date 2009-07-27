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
#include "modules/editors.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "raster/image.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"

static void cmd_eyedropper_tool_execute(const char *argument)
{
  JWidget widget;
  Editor *editor;
  color_t color;
  int x, y;

  widget = jmanager_get_mouse();
  if (!widget || widget->type != editor_type())
    return;

  editor = editor_data(widget);
  if (!editor->sprite)
    return;

  /* pixel position to get */
  screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

  /* get the color from the image */
  color = color_from_image(editor->sprite->imgtype,
			   sprite_getpixel(editor->sprite, x, y));

  if (color_type(color) != COLOR_TYPE_MASK) {
    /* set the color of the color-bar */
    if (ustrcmp(argument, "background") == 0)
      colorbar_set_bg_color(app_get_colorbar(), color);
    else
      colorbar_set_fg_color(app_get_colorbar(), color);
  }
}

Command cmd_eyedropper_tool = {
  CMD_EYEDROPPER_TOOL,
  NULL,
  NULL,
  cmd_eyedropper_tool_execute,
};
