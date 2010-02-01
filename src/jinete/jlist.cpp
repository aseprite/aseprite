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

/* #define USE_JUNKLIST */

#include "config.h"

#include "jinete/jlist.h"

#ifdef USE_JUNKLIST		/* TODO warning not thread safe */
static JList junklist = NULL;
#endif

JLink jlink_new(void *data)
{
#ifdef USE_JUNKLIST
  JLink link;
  if (!junklist || jlist_empty(junklist))
    link = jnew0(struct jlink, 1);
  else {
    link = jlist_first(junklist);
    jlist_remove_link(junklist, link);
  }
#else
  JLink link = jnew0(struct jlink, 1);
#endif
  link->data = data;
  return link;
}

JList jlist_new()
{
  JList list = jnew0(struct jlist, 1);
  list->end = jlink_new(NULL);
  list->end->prev = list->end;
  list->end->next = list->end;
  list->length = 0;
  return list;
}

void jlink_free(JLink link)
{
#ifdef USE_JUNKLIST
  if (!junklist)
    junklist = jlist_new();
  link->prev = junklist->end->prev;
  link->next = junklist->end;
  link->prev->next = link;
  link->next->prev = link;
  junklist->length++;
#else
  jfree(link);
#endif
}

void jlist_free(JList list)
{
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(list, link, next) {
    jlink_free(link);
  }
  jlink_free(list->end);
  jfree(list);
}

void jlist_clear(JList list)
{
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(list, link, next) {
    jlink_free(link);
  }
  list->end->prev = list->end;
  list->end->next = list->end;
  list->length = 0;
}

void jlist_append(JList list, void *data)
{
  JLink link = jlink_new(data);
  link->prev = list->end->prev;
  link->next = list->end;
  link->prev->next = link;
  link->next->prev = link;
  list->length++;
}

void jlist_prepend(JList list, void *data)
{
  JLink link = jlink_new(data);
  link->prev = list->end;
  link->next = list->end->next;
  link->prev->next = link;
  link->next->prev = link;
  list->length++;
}

void jlist_insert(JList list, void *data, int position)
{
  JLink new_link, tmp_link;

  if (position < 0) {
    jlist_append(list, data);
    return;
  }
  else if (position == 0) {
    jlist_prepend(list, data);
    return;
  }

  tmp_link = jlist_nth_link(list, position);
  if (!tmp_link) {
    jlist_append(list, data);
    return;
  }

  new_link = jlink_new(data);
  new_link->prev = tmp_link->prev;
  new_link->next = tmp_link;
  new_link->prev->next = new_link;
  new_link->next->prev = new_link;
  list->length++;
}

void jlist_insert_before(JList list, JLink sibling, void *data)
{
  if (jlist_empty(list))
    jlist_prepend(list, data);
  else if (sibling) {
    JLink new_link;

    new_link = jlink_new(data);
    new_link->prev = sibling->prev;
    new_link->next = sibling;
    new_link->prev->next = new_link;
    new_link->next->prev = new_link;
    list->length++;
  }
  else
    jlist_append(list, data);
}

void jlist_remove(JList list, const void *data)
{
  JLink link;
  JI_LIST_FOR_EACH(list, link) {
    if (link->data == data) {
      link->prev->next = link->next;
      link->next->prev = link->prev;
      jlink_free(link);
      list->length--;
      return;
    }
  }
}

void jlist_remove_all(JList list, const void *data)
{
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(list, link, next) {
    if (link->data == data) {
      link->prev->next = link->next;
      link->next->prev = link->prev;
      jlink_free(link);
      list->length--;
    }
  }
}

void jlist_remove_link(JList list, JLink link)
{
  link->prev->next = link->next;
  link->next->prev = link->prev;
  list->length--;
}

void jlist_delete_link(JList list, JLink link)
{
  link->prev->next = link->next;
  link->next->prev = link->prev;
  jlink_free(link);
  list->length--;
}

JList jlist_copy(JList list)
{
  JList new_list = jlist_new();
  JLink new_link, link;

  JI_LIST_FOR_EACH(list, link) {
    /* it's like jlist_append(new_list, link->data) */
    new_link = jlink_new(link->data);
    new_link->prev = new_list->end->prev;
    new_link->next = new_list->end;
    new_link->prev->next = new_link;
    new_link->next->prev = new_link;
  }

  new_list->length = list->length;
  return new_list;
}

JLink jlist_nth_link(JList list, unsigned int n)
{
  JLink link;
  JI_LIST_FOR_EACH(list, link) {
    if (n-- <= 0)
      return link;
  }
  return list->end;
}

void *jlist_nth_data(JList list, unsigned int n)
{
  JLink link;
  JI_LIST_FOR_EACH(list, link) {
    if (n-- <= 0)
      return link->data;
  }
  return NULL;
}

JLink jlist_find(JList list, const void *data)
{
  JLink link;
  JI_LIST_FOR_EACH(list, link) {
    if (link->data == data)
      return link;
  }
  return list->end;
}
