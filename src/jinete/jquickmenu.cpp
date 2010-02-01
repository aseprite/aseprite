/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "jinete/jaccel.h"
#include "jinete/jlist.h"
#include "jinete/jmenu.h"
#include "jinete/jmessage.h"
#include "jinete/jquickmenu.h"
#include "jinete/jsep.h"
#include "jinete/jwidget.h"

typedef struct QuickData
{
  void (*quick_handler)(JWidget widget, int user_data);
  int user_data;
} QuickData;

static void process_quickmenu(JWidget menu, JQuickMenu quick_menu);
static bool quickmenu_msg_proc(JWidget widget, JMessage msg);

static int quickmenu_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

JWidget jmenubar_new_quickmenu(JQuickMenu quick_menu)
{
  JWidget menubar = jmenubar_new();
  JWidget menu = jmenu_new();

  jmenubar_set_menu(menubar, menu);
  process_quickmenu(menu, quick_menu);

  return menubar;
}

JWidget jmenubox_new_quickmenu(JQuickMenu quick_menu)
{
  JWidget menubox = jmenubox_new();
  JWidget menu = jmenu_new();

  jmenubox_set_menu(menubox, menu);
  process_quickmenu(menu, quick_menu);

  return menubox;
}

static void process_quickmenu(JWidget menu, JQuickMenu quick_menu)
{
  JWidget menuitem = NULL;
  JList parent = jlist_new();
  int c, old_level = 0;

  jlist_append(parent, menu);

  for (c=0; quick_menu[c].level >= 0; c++) {
    if (old_level < quick_menu[c].level) {
      if (menuitem) {
        menu = jmenu_new();
        jmenuitem_set_submenu(menuitem, menu);
        jlist_append(parent, menu);
      }
    }
    else {
      while (old_level > quick_menu[c].level) {
        old_level--;
        jlist_remove(parent, jlist_last(parent)->data);
      }
    }

    /* normal menu item */
    if (quick_menu[c].text) {
      menuitem = jmenuitem_new(quick_menu[c].text);

      if (quick_menu[c].accel) {
        JAccel accel = jaccel_new();
        jaccel_add_keys_from_string(accel, quick_menu[c].accel);
        jmenuitem_set_accel(menuitem, accel);
      }

      if (quick_menu[c].quick_handler) {
	QuickData *quick_data = jnew(QuickData, 1);

	quick_data->quick_handler = quick_menu[c].quick_handler;
	quick_data->user_data = quick_menu[c].user_data;

	jwidget_add_hook(menuitem, quickmenu_type(),
			 quickmenu_msg_proc, quick_data);
      }
    }
    /* separator */
    else {
      menuitem = ji_separator_new(NULL, JI_HORIZONTAL);
    }

    jwidget_add_child((JWidget)jlist_last(parent)->data, menuitem);

    old_level = quick_menu[c].level;
  }

  jlist_free(parent);
}

static bool quickmenu_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      jfree(jwidget_get_data(widget, quickmenu_type()));
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
	QuickData* quick_data = reinterpret_cast<QuickData*>(jwidget_get_data(widget, quickmenu_type()));
	(*quick_data->quick_handler)(widget, quick_data->user_data);
	return true;
      }
      break;
  }

  return false;
}

