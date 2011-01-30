// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/list.h"

JList jlist_new()
{
  JList list = new jlist;
  list->end = new jlink(NULL);
  list->end->prev = list->end;
  list->end->next = list->end;
  list->length = 0;
  return list;
}

void jlist_free(JList list)
{
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(list, link, next)
    delete link;
  delete list->end;
  delete list;
}

void jlist_clear(JList list)
{
  JLink link, next;
  JI_LIST_FOR_EACH_SAFE(list, link, next) {
    delete link;
  }
  list->end->prev = list->end;
  list->end->next = list->end;
  list->length = 0;
}

void jlist_append(JList list, void *data)
{
  JLink link = new jlink(data);
  link->prev = list->end->prev;
  link->next = list->end;
  link->prev->next = link;
  link->next->prev = link;
  list->length++;
}

void jlist_prepend(JList list, void *data)
{
  JLink link = new jlink(data);
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

  new_link = new jlink(data);
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

    new_link = new jlink(data);
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
      delete link;
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
      delete link;
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
  delete link;
  list->length--;
}

JList jlist_copy(JList list)
{
  JList new_list = jlist_new();
  JLink new_link, link;

  JI_LIST_FOR_EACH(list, link) {
    /* it's like jlist_append(new_list, link->data) */
    new_link = new jlink(link->data);
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
