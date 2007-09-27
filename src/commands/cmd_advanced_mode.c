/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "commands/commands.h"
#include "core/app.h"
#include "core/cfg.h"

#endif

static bool advanced_mode = FALSE;

void command_execute_advanced_mode(const char *argument)
{
  advanced_mode = !advanced_mode;

  if (advanced_mode) {
    jwidget_hide(app_get_tool_bar());
    jwidget_hide(app_get_menu_bar());
    jwidget_hide(app_get_status_bar());
    jwidget_hide(app_get_color_bar());
  }
  else {
    jwidget_show(app_get_tool_bar());
    jwidget_show(app_get_menu_bar());
    jwidget_show(app_get_status_bar());
    jwidget_show(app_get_color_bar());
  }

  jwindow_remap(app_get_top_window());
  jwidget_dirty(app_get_top_window());
  
  if (advanced_mode &&
      get_config_bool("AdvancedMode", "Warning", TRUE)) {
    Command *cmd_advanced_mode = command_get_by_name(CMD_ADVANCED_MODE);
    char warning[1024];
    char key[1024];
    char buf[1024];

    strcpy(warning, _("You are going to enter in \"Advanced Mode\"."
		      "<<You can back pressing the \"%s\" key."));
    jaccel_to_string(cmd_advanced_mode->accel, key);

    sprintf(buf, warning, key);
    
    if (jalert("%s<<%s||%s||%s",
	       _("Warning - Important"),
	       buf,
	       _("&Don't show it again"), _("&Continue")) == 1) {
      set_config_bool("AdvancedMode", "Warning", FALSE);
    }
  }
}
