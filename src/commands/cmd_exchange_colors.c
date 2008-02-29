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

#include <allegro/unicode.h>

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "core/app.h"
#include "widgets/colbar.h"

static void cmd_exchange_colors_execute(const char *argument)
{
  JWidget colorbar = app_get_colorbar();
  color_t fg = colorbar_get_fg_color(colorbar);
  color_t bg = colorbar_get_bg_color(colorbar);

  colorbar_set_fg_color(colorbar, bg);
  colorbar_set_bg_color(colorbar, fg);
}

Command cmd_exchange_colors = {
  CMD_EXCHANGE_COLORS,
  NULL,
  NULL,
  cmd_exchange_colors_execute,
  NULL
};
