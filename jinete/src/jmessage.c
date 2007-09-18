/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro/keyboard.h>
#include <string.h>

#include "jinete/list.h"
#include "jinete/manager.h"
#include "jinete/message.h"
#include "jinete/rect.h"
#include "jinete/widget.h"

JMessage jmessage_new(int type)
{
  JMessage msg;

  msg = jnew0(union jmessage, 1);
  if (!msg)
    return NULL;

  msg->type = type;
  msg->any.widgets = jlist_new();
  msg->any.shifts = key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG);

/*   printf("type=%02d ", type); */
/*   if (key_shifts & KB_SHIFT_FLAG) */
/*     printf("KB_SHIFT_FLAG "); */
/*   if (key_shifts & KB_CTRL_FLAG) */
/*     printf("KB_CTRL_FLAG "); */
/*   if (key_shifts & KB_ALT_FLAG) */
/*     printf("KB_ALT_FLAG "); */
/*   printf("\n"); */

  return msg;
}

JMessage jmessage_new_copy(const JMessage msg)
{
  JMessage copy;

  copy = jnew(union jmessage, 1);
  if (!copy)
    return NULL;

  memcpy(copy, msg, sizeof(union jmessage));

  copy->any.widgets = jlist_copy(msg->any.widgets);
  copy->any.sub_msg = msg->any.sub_msg ?
    jmessage_new_copy(msg->any.sub_msg): NULL;
  copy->any.used = FALSE;

  return copy;
}

void jmessage_free(JMessage msg)
{
  if (msg->any.sub_msg)
    jmessage_free(msg->any.sub_msg);

  jlist_free(msg->any.widgets);
  jfree(msg);
}

void jmessage_add_dest(JMessage msg, JWidget widget)
{
  jlist_append(msg->any.widgets, widget);
}

void jmessage_broadcast_to_children(JMessage msg, JWidget widget)
{
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link)
    jmessage_broadcast_to_children(msg, link->data);

  jmessage_add_dest(msg, widget);
}

void jmessage_broadcast_to_parents(JMessage msg, JWidget widget)
{
  if (widget && widget->type != JI_MANAGER) {
    jmessage_add_dest(msg, widget);
    jmessage_broadcast_to_parents(msg, jwidget_get_parent(widget));
  }
}

void jmessage_set_sub_msg(JMessage msg, JMessage sub_msg)
{
  if (msg->any.sub_msg)
    jmessage_free(msg->any.sub_msg);

  msg->any.sub_msg = sub_msg ? jmessage_new_copy(sub_msg): NULL;
}
