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

#include "core/core.h"
#include "modules/chkmthds.h"
#include "modules/gui.h"
#include "script/script.h"

#endif

typedef struct MenuItem
{
  char *script;
  CheckMethod check_method;
} MenuItem;

static int menuitem_type (void);
static bool menuitem_msg_proc (JWidget widget, JMessage msg);

/**
 * A widget that represent a menu item of the application.
 *
 * It's like a jmenuitem, but it has a extra properties: the script to
 * be executed and a method to check the status of the menu-item.
 * 
 * @see jmenuitem_new
 */
JWidget menuitem_new(const char *text)
{
  JWidget widget = jmenuitem_new(text);
  MenuItem *menuitem = jnew0(MenuItem, 1);

  jwidget_add_hook(widget, menuitem_type(),
		   menuitem_msg_proc, menuitem);

  return widget;
}

const char *menuitem_get_script(JWidget widget)
{
  MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

  return menuitem->script;
}

void menuitem_set_script(JWidget widget, const char *format, ...)
{
  MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());
  char buf[4096];
  va_list ap;

  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  menuitem->script = jstrdup(buf);
}

void menuitem_set_check_method(JWidget widget, CheckMethod check_method)
{
  MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

  menuitem->check_method = check_method;
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
      MenuItem *menuitem = jwidget_get_data (widget, menuitem_type ());

      if (menuitem->script) {
	jfree(menuitem->script);
	menuitem->script = NULL;
      }

      jfree(menuitem);
      break;
    }

    case JM_OPEN: {
      MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

      if (menuitem->check_method) {
	/* call the check method */
	if ((*menuitem->check_method)(widget))
	  /* enable the widget */
	  jwidget_enable(widget);
	else
	  /* disable it */
	  jwidget_disable(widget);
      }
      break;
    }

    case JM_CLOSE: {
      MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

      /* enable (to get the keyboard bindinds) */
      if (menuitem->check_method)
	jwidget_enable(widget);
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
	/* call the script of the selected menu item */
	MenuItem *menuitem = jwidget_get_data(widget, menuitem_type());

	if (menuitem->script) {
	  if (menuitem->check_method) {
	    /* call the check method */
	    if (!(*menuitem->check_method)(widget))
	      return FALSE;
	  }

	  PRINTF("Calling menu \"%s\" script\n", widget->text);

	  rebuild_lock();
	  do_script_expr(menuitem->script);
	  rebuild_unlock();

	  PRINTF("Done with menu call\n");
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}
