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
#include <allegro/unicode.h>

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jlist.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#include "modules/gui.h"
#include "widgets/groupbut.h"

static JWidget find_selected(JWidget widget);
static int select_button(JWidget widget, int index);

static bool radio_change_hook(JWidget widget, void *data);

JWidget group_button_new(int w, int h, int first_selected, ...)
{
  JWidget vbox = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  JWidget hbox = NULL;
  JWidget radio;
  int x, y, icon;
  va_list ap;
  int c = 0;
  char buf[256];

  va_start(ap, first_selected);

  jwidget_noborders(vbox);

  for (y=0; y<h; y++) {
    if (w > 1) {
      hbox = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
      jwidget_noborders(hbox);
    }

    for (x=0; x<w; x++) {
      icon = va_arg(ap, int);

      radio = radio_button_new(vbox->id+0x1000,
			       x ==   0 && y ==   0 ? 2: 0,
			       x == w-1 && y ==   0 ? 2: 0,
			       x ==   0 && y == h-1 ? 2: 0,
			       x == w-1 && y == h-1 ? 2: 0);

      radio->user_data[1] = (void *)c;

      usprintf(buf, "radio%d", c);
      radio->setName(buf);

      if (icon >= 0)
	add_gfxicon_to_button(radio, icon, JI_CENTER | JI_MIDDLE);

      HOOK(radio, JI_SIGNAL_RADIO_CHANGE, radio_change_hook, vbox);

      if (c == first_selected)
	radio->setSelected(true);

      if (hbox)
	jwidget_add_child(hbox, radio);
      else
	jwidget_add_child(vbox, radio);

      c++;
    }

    if (hbox)
      jwidget_add_child(vbox, hbox);
  }

  va_end(ap);

  return vbox;
}

int group_button_get_selected(JWidget group)
{
  JWidget sel = find_selected(group);

  if (sel)
    return (int)sel->user_data[1];
  else
    return -1;
}

void group_button_select(JWidget group, int index)
{
  JWidget sel = find_selected(group);

  if (!sel || (int)sel->user_data[1] != index) {
    sel->setSelected(false);
    select_button(group, index);
  }
}

static JWidget find_selected(JWidget widget)
{
  JWidget sel;
  JLink link;

  if (widget->isSelected())
    return widget;
  else {
    JI_LIST_FOR_EACH(widget->children, link)
      if ((sel = find_selected(reinterpret_cast<JWidget>(link->data))))
	return sel;

    return NULL;
  }
}

static int select_button(JWidget widget, int index)
{
  JLink link;

  if (widget->type == JI_RADIO) {
    if ((int)widget->user_data[1] == index) {
      widget->setSelected(true);
      return true;
    }
  }
  else {
    JI_LIST_FOR_EACH(widget->children, link)
      if (select_button(reinterpret_cast<JWidget>(link->data), index))
	return true;
  }

  return false;
}

static bool radio_change_hook(JWidget widget, void *data)
{
  jwidget_emit_signal((JWidget)data, SIGNAL_GROUP_BUTTON_CHANGE);
  return true;
}
