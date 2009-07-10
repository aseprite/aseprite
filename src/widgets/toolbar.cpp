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

#include "jinete/jaccel.h"
#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jtooltips.h"
#include "jinete/jwidget.h"

#include "commands/commands.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/tools.h"
#include "widgets/groupbut.h"
#include "widgets/toolbar.h"

static bool tools_change_hook(JWidget widget, void *data);
static void conf_command(JWidget widget);

JWidget toolbar_new()
{
  JWidget box, confbutton, tools;
  char buf[1024];		/* TODO warning buffer overflow */
  int c;

  for (c=0; c<MAX_TOOLS; c++)
    if (current_tool == tools_list[c])
      break;

  box = jbox_new(JI_VERTICAL);
  confbutton = jbutton_new(NULL);

  add_gfxicon_to_button(confbutton, GFX_TOOL_CONFIGURATION,
			JI_CENTER | JI_MIDDLE);

  tools = group_button_new(1, MAX_TOOLS, c,
			   GFX_TOOL_MARKER,
			   GFX_TOOL_PENCIL,
			   GFX_TOOL_BRUSH,
			   GFX_TOOL_ERASER,
			   GFX_TOOL_FLOODFILL,
			   GFX_TOOL_SPRAY,
			   GFX_TOOL_LINE,
			   GFX_TOOL_CURVE,
			   GFX_TOOL_RECTANGLE,
			   GFX_TOOL_ELLIPSE,
			   GFX_TOOL_BLUR,
			   GFX_TOOL_JUMBLE);

  for (c=0; c<MAX_TOOLS; c++) {
    JWidget child;
    usprintf(buf, "radio%d", c);
    child = jwidget_find_name(tools, buf);

    usprintf(buf, "%s", _(tools_list[c]->tips));

    JAccel accel = get_accel_to_change_tool(tools_list[c]);
    if (accel) {
      ustrcat(buf, "\n(");
      jaccel_to_string(accel, buf+ustrsize(buf));
      ustrcat(buf, ")");
    }

    jwidget_add_tooltip_text(child, buf);
  }
  
  jwidget_expansive(box, TRUE);

  jwidget_add_child(box, tools);
  jwidget_add_child(box, confbutton);

  HOOK(tools, SIGNAL_GROUP_BUTTON_CHANGE, tools_change_hook, 0);
  jbutton_add_command(confbutton, conf_command);

  box->user_data[0] = tools;

  return box;
}

void toolbar_update(JWidget widget)
{
  JWidget group = reinterpret_cast<JWidget>(widget->user_data[0]);
  int c;

  for (c=0; c<MAX_TOOLS; c++)
    if (current_tool == tools_list[c])
      break;

  group_button_select(group, c);
}

static bool tools_change_hook(JWidget widget, void *data)
{
  int c = group_button_get_selected(widget);

  if (current_tool != tools_list[c])
    select_tool(tools_list[c]);

  return FALSE;
}

static void conf_command(JWidget widget)
{
  command_execute(command_get_by_name(CMD_CONFIGURE_TOOLS), NULL);
}
