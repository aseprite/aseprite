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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "jinete/hook.h"
#include "jinete/menu.h"
#include "jinete/message.h"
#include "jinete/widget.h"

#include "commands/commands.h"
#include "core/core.h"
#include "modules/chkmthds.h"
#include "modules/gui.h"
#include "script/script.h"

#endif

typedef struct MenuItem
{
  Command *command;
  char *argument;
} MenuItem;

static int menuitem_type(void);
static bool menuitem_msg_proc(JWidget widget, JMessage msg);

/**
 * A widget that represent a menu item of the application.
 *
 * It's like a jmenuitem, but it has a extra properties: the name of
 * the command to be executed when it's clicked (also that command is
 * used to check the availability of the command).
 * 
 * @see jmenuitem_new
 */
JWidget menuitem_new(const char *text,
		     Command *command,
		     const char *argument)
{
  JWidget widget = jmenuitem_new(text);
  MenuItem *menuitem = jnew0(MenuItem, 1);

  menuitem->command = command;
  menuitem->argument = argument ? jstrdup(argument): NULL;

  jwidget_add_hook(widget,
		   menuitem_type(),
		   menuitem_msg_proc,
		   menuitem);

  return widget;
}

Command *menuitem_get_command(JWidget widget)
{
  MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());
  return menuitem->command;
}

static int menuitem_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool menuitem_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY: {
      MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());
      if (menuitem->argument)
	jfree(menuitem->argument);
      jfree(menuitem);
      break;
    }

    case JM_OPEN: {
      MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

      if (menuitem->command) {
	/* enabled? */
	if (command_is_enabled(menuitem->command,
			       menuitem->argument))
	  jwidget_enable(widget);
	else
	  jwidget_disable(widget);

	/* selected? */
	if (command_is_selected(menuitem->command,
				menuitem->argument))
	  jwidget_select(widget);
	else
	  jwidget_deselect(widget);
      }
      break;
    }

    case JM_CLOSE: {
      MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

      /* disable the menu (the keyboard shortcuts are processed by "manager_msg_proc") */
      if (menuitem->command)
	jwidget_disable(widget);
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
	MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

	if (menuitem->command && command_is_enabled(menuitem->command,
						    menuitem->argument)) {
	  command_execute(menuitem->command,
			  menuitem->argument);
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}
