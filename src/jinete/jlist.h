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

#ifndef JINETE_JLIST_H_INCLUDED
#define JINETE_JLIST_H_INCLUDED

#include "jinete/jbase.h"

struct jlink
{
  void *data;
  JLink prev;
  JLink next;
};

struct jlist
{
  JLink end;
  unsigned int length;
};

JLink jlink_new(void *data);
JList jlist_new();
void jlink_free(JLink link);
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

#define jlist_first(list)		(((JList)(list))->end->next)
#define jlist_last(list)		(((JList)(list))->end->prev)
#define jlist_length(list)		(((JList)(list))->length)

#define jlist_first_data(list)		(jlist_first(list)->data)
#define jlist_last_data(list)		(jlist_last(list)->data)

#define jlist_empty(list)		(((JList)(list))->length == 0)

#define JI_LIST_FOR_EACH(list, link)		\
  for (link=((JList)(list))->end->next;		\
       link!=((JList)(list))->end;		\
       link=link->next)

#define JI_LIST_FOR_EACH_BACK(list, link)	\
  for (link=((JList)(list))->end->prev;		\
       link!=((JList)(list))->end;		\
       link=link->prev)

/**
 * Iterator for each item of the list (the body of the "for" can be
 * remove elements in the list).
 */
#define JI_LIST_FOR_EACH_SAFE(list, link, next)			\
  for (link=((JList)(list))->end->next, next=link->next;	\
       link!=((JList)(list))->end;				\
       link=next, next=link->next)

#endif
