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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "jinete/jhook.h"
#include "jinete/jmenu.h"
#include "jinete/jmessage.h"
#include "jinete/jwidget.h"

#include "commands/commands.h"
#include "core/core.h"
#include "modules/gui.h"

typedef struct MenuItem
{
  Command *command;
  char *argument;
} MenuItem;

static int menuitem_type();
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
JWidget menuitem_new(const char* text,
		     Command* command,
		     const char* argument)
{
  JWidget widget = jmenuitem_new(text);
  MenuItem *menuitem = jnew0(MenuItem, 1);

  menuitem->command = command;
  menuitem->argument = jstrdup(argument ? argument: "");

  jwidget_add_hook(widget,
		   menuitem_type(),
		   menuitem_msg_proc,
		   menuitem);

  return widget;
}

Command* menuitem_get_command(JWidget widget)
{
  MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
  return menuitem->command;
}

const char* menuitem_get_argument(JWidget widget)
{
  MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
  return menuitem->argument;
}

static int menuitem_type()
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
      MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
      jfree(menuitem->argument);
      jfree(menuitem);
      break;
    }

    case JM_OPEN: {
      MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));

      if (menuitem->command) {
	/* enabled? */
	if (command_is_enabled(menuitem->command, menuitem->argument))
	  jwidget_enable(widget);
	else
	  jwidget_disable(widget);

	/* selected? */
	if (command_is_checked(menuitem->command, menuitem->argument))
	  jwidget_select(widget);
	else
	  jwidget_deselect(widget);
      }
      break;
    }

    case JM_CLOSE: {
      MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));

      /* disable the menu (the keyboard shortcuts are processed by "manager_msg_proc") */
      if (menuitem->command)
	jwidget_disable(widget);
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
	MenuItem *menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));

	if (menuitem->command && command_is_enabled(menuitem->command,
						    menuitem->argument)) {
	  command_execute(menuitem->command, menuitem->argument);
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}
