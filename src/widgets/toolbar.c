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

#include "jinete/box.h"
#include "jinete/button.h"
#include "jinete/hook.h"
#include "jinete/widget.h"

#include "commands/commands.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/tools.h"
#include "script/script.h"
#include "widgets/groupbut.h"
#include "widgets/toolbar.h"

#endif

static int group_change_signal(JWidget widget, int user_data);
static void conf_command(JWidget widget);

JWidget tool_bar_new(int align)
{
#define ICONS_LIST				\
  GFX_TOOL_MARKER,				\
  GFX_TOOL_DOTS,				\
  GFX_TOOL_PENCIL,				\
  GFX_TOOL_BRUSH,				\
  GFX_TOOL_FLOODFILL,				\
  GFX_TOOL_SPRAY,				\
  GFX_TOOL_LINE,				\
  GFX_TOOL_RECTANGLE,				\
  GFX_TOOL_ELLIPSE

  JWidget box, fillbox, confbutton, group;
  int c, len;

  for (c=0; ase_tools_list[c]; c++)
    ;
  len = c;

  for (c=0; ase_tools_list[c]; c++)
    if (current_tool == ase_tools_list[c])
      break;

  box = jbox_new(align);
  fillbox = jbox_new(align);
  confbutton = jbutton_new(NULL);

  add_gfxicon_to_button(confbutton, GFX_TOOL_CONFIGURATION,
			JI_CENTER | JI_MIDDLE);

  if (align == JI_HORIZONTAL)
    group = group_button_new(len, 1, c, ICONS_LIST);
  else {
/*     group = group_button_new(1, len, c, ICONS_LIST); */
    group = group_button_new(1, len, c, ICONS_LIST);
  }

  jwidget_expansive(box, TRUE);
  jwidget_expansive(fillbox, TRUE);

  jwidget_add_child(box, group);
  jwidget_add_child(box, fillbox);
  jwidget_add_child(box, confbutton);

  HOOK(group, SIGNAL_GROUP_BUTTON_CHANGE, group_change_signal, 0);
  jbutton_add_command(confbutton, conf_command);

  box->user_data[0] = group;

  return box;
}

void tool_bar_update(JWidget widget)
{
  JWidget group = widget->user_data[0];
  int c;

  for (c=0; ase_tools_list[c]; c++)
    if (current_tool == ase_tools_list[c])
      break;

  group_button_select(group, c);
}

static int group_change_signal(JWidget widget, int user_data)
{
  int c = group_button_get_selected(widget);

  if (current_tool != ase_tools_list[c])
    select_tool(ase_tools_list[c]);

  return FALSE;
}

static void conf_command(JWidget widget)
{
  command_execute(command_get_by_name(CMD_CONFIGURE_TOOLS), NULL);
}
