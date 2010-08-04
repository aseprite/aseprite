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

#include <allegro/keyboard.h>
#include <string.h>

#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jwidget.h"

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

JMessage jmessage_new_key_related(int type, int readkey_value)
{
  JMessage msg = jmessage_new(type);

  msg->key.scancode = (readkey_value >> 8) & 0xff;
  msg->key.ascii = readkey_value & 0xff;
  msg->key.repeat = 0;
  msg->key.propagate_to_children = false;
  msg->key.propagate_to_parent = true;

#if 0
  printf("%i: %i %i [%c]\n", type, msg->key.scancode,
	 msg->key.ascii, msg->key.ascii);
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
