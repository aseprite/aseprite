// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_LIST_H_INCLUDED
#define GUI_LIST_H_INCLUDED

#include "gui/base.h"

struct jlink
{
  void* data;
  JLink prev;
  JLink next;

  jlink(void* data) : data(data), prev(0), next(0) { }
};

struct jlist
{
  JLink end;
  unsigned int length;

  jlist() : end(0), length(0) { }
};

JList jlist_new();
void jlist_free(JList list);
void jlist_clear(JList list);
void jlist_append(JList list, void *data);
void jlist_prepend(JList list, void *data);
void jlist_insert(JList list, void *data, int position);
void jlist_insert_before(JList list, JLink sibling, void *data);
void jlist_remove(JList list, const void *data);
void jlist_remove_all(JList list, const void *data);
void jlist_remove_link(JList list, JLink link);
void jlist_delete_link(JList list, JLink link);
JList jlist_copy(JList list);
JLink jlist_nth_link(JList list, unsigned int n);
void *jlist_nth_data(JList list, unsigned int n);
JLink jlist_find(JList list, const void *data);

#define jlist_first(list)               (((JList)(list))->end->next)
#define jlist_last(list)                (((JList)(list))->end->prev)
#define jlist_length(list)              (((JList)(list))->length)

#define jlist_first_data(list)          (jlist_first(list)->data)
#define jlist_last_data(list)           (jlist_last(list)->data)

#define jlist_empty(list)               (((JList)(list))->length == 0)

#define JI_LIST_FOR_EACH(list, link)            \
  for (link=((JList)(list))->end->next;         \
       link!=((JList)(list))->end;              \
       link=link->next)

#define JI_LIST_FOR_EACH_BACK(list, link)       \
  for (link=((JList)(list))->end->prev;         \
       link!=((JList)(list))->end;              \
       link=link->prev)

/**
 * Iterator for each item of the list (the body of the "for" can be
 * remove elements in the list).
 */
#define JI_LIST_FOR_EACH_SAFE(list, link, next)                 \
  for (link=((JList)(list))->end->next, next=link->next;        \
       link!=((JList)(list))->end;                              \
       link=next, next=link->next)

#endif
