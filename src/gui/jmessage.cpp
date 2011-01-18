// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/keyboard.h>
#include <string.h>

#include "gui/jlist.h"
#include "gui/jmanager.h"
#include "gui/jmessage.h"
#include "gui/jrect.h"
#include "gui/widget.h"

static int registered_messages = JM_REGISTERED_MESSAGES;

int ji_register_message_type()
{
  return registered_messages++;
}

JMessage jmessage_new(int type)
{
  JMessage msg;

  msg = jnew0(union jmessage, 1);
  if (!msg)
    return NULL;

  msg->type = type;
  msg->any.widgets = jlist_new();
  msg->any.shifts =
    (key[KEY_LSHIFT] || key[KEY_RSHIFT] ? KB_SHIFT_FLAG: 0) |
    (key[KEY_LCONTROL] || key[KEY_RCONTROL] ? KB_CTRL_FLAG: 0) |
    (key[KEY_ALT] ? KB_ALT_FLAG: 0);

  // printf("type=%02d ", type);
  // if (msg->any.shifts & KB_SHIFT_FLAG) printf("KB_SHIFT_FLAG ");
  // if (msg->any.shifts & KB_CTRL_FLAG) printf("KB_CTRL_FLAG ");
  // if (msg->any.shifts & KB_ALT_FLAG) printf("KB_ALT_FLAG ");
  // printf("\n");
  // fflush(stdout);

  return msg;
}

JMessage jmessage_new_key_related(int type, int readkey_value)
{
  JMessage msg = jmessage_new(type);

  msg->key.scancode = (readkey_value >> 8) & 0xff;
  msg->key.ascii = readkey_value & 0xff;
  msg->key.repeat = 0;
  msg->key.propagate_to_children = false;
  msg->key.propagate_to_parent = true;

#if 0
  printf("%s: %i %i [%c]\n", type == JM_KEYPRESSED ? "JM_KEYPRESSED":
						     "JM_KEYRELEASED",
	 msg->key.scancode, msg->key.ascii, msg->key.ascii);
  fflush(stdout);
#endif
  return msg;
}

JMessage jmessage_new_copy(const JMessage msg)
{
  JMessage copy;

  ASSERT(msg != NULL);

  copy = jnew(union jmessage, 1);
  if (!copy)
    return NULL;

  memcpy(copy, msg, sizeof(union jmessage));

  copy->any.widgets = jlist_copy(msg->any.widgets);
  copy->any.used = false;

  return copy;
}

JMessage jmessage_new_copy_without_dests(const JMessage msg)
{
  JMessage copy;

  ASSERT(msg != NULL);

  copy = jnew(union jmessage, 1);
  if (!copy)
    return NULL;

  memcpy(copy, msg, sizeof(union jmessage));

  copy->any.widgets = jlist_new();
  copy->any.used = false;

  return copy;
}

void jmessage_free(JMessage msg)
{
  ASSERT(msg != NULL);

  jlist_free(msg->any.widgets);
  jfree(msg);
}

void jmessage_add_dest(JMessage msg, JWidget widget)
{
  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  jlist_append(msg->any.widgets, widget);
}

void jmessage_add_pre_dest(JMessage msg, JWidget widget)
{
  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  jlist_prepend(msg->any.widgets, widget);
}

void jmessage_broadcast_to_children(JMessage msg, JWidget widget)
{
  JLink link;

  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  JI_LIST_FOR_EACH(widget->children, link)
    jmessage_broadcast_to_children(msg, reinterpret_cast<JWidget>(link->data));

  jmessage_add_dest(msg, widget);
}

void jmessage_broadcast_to_parents(JMessage msg, JWidget widget)
{
  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  if (widget && widget->type != JI_MANAGER) {
    jmessage_add_dest(msg, widget);
    jmessage_broadcast_to_parents(msg, jwidget_get_parent(widget));
  }
}
