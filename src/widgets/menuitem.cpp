/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "ui_context.h"
#include "commands/command.h"
#include "commands/params.h"
#include "core/core.h"
#include "modules/gui.h"

struct MenuItem
{
  Command *m_command;
  Params *m_params;

  MenuItem(Command *command, Params* params)
    : m_command(command)
    , m_params(params ? params->clone(): NULL)
  {
  }

  ~MenuItem()
  {
    delete m_params;
  }

};

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
JWidget menuitem_new(const char* text, Command* command, Params* params)
{
  JWidget widget = jmenuitem_new(text);
  MenuItem* menuitem = new MenuItem(command, params);

  jwidget_add_hook(widget,
		   menuitem_type(),
		   menuitem_msg_proc,
		   menuitem);

  return widget;
}

Command* menuitem_get_command(JWidget widget)
{
  MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
  return menuitem->m_command;
}

Params* menuitem_get_params(JWidget widget)
{
  MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
  return menuitem->m_params;
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
      delete menuitem;
      break;
    }

    case JM_OPEN: {
      MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
      UIContext* context = UIContext::instance();

      if (menuitem->m_command) {
	if (menuitem->m_params)
	  menuitem->m_command->loadParams(menuitem->m_params);

	widget->setEnabled(menuitem->m_command->isEnabled(context));
	widget->setSelected(menuitem->m_command->isChecked(context));
      }
      break;
    }

    case JM_CLOSE:
      // disable the menu (the keyboard shortcuts are processed by "manager_msg_proc")
      widget->setEnabled(false);
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
	MenuItem* menuitem = reinterpret_cast<MenuItem*>(jwidget_get_data(widget, menuitem_type()));
	UIContext* context = UIContext::instance();

	if (menuitem->m_command) {
	  if (menuitem->m_params)
	    menuitem->m_command->loadParams(menuitem->m_params);

	  if (menuitem->m_command->isEnabled(context)) {
	    context->execute_command(menuitem->m_command);
	    return true;
	  }
	}
      }
      break;
  }

  return false;
}
