/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_LIST_H
#define JINETE_LIST_H

#include "jinete/base.h"

JI_BEGIN_DECLS

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
JList jlist_new(void);
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

#define jlist_empty(list)		(!((JList)(list))->length)

#define JI_LIST_FOR_EACH(list, link)		\
  for (link=((JList)(list))->end->next;		\
       link!=((JList)(list))->end;		\
       link=link->next)

#define JI_LIST_FOR_EACH_BACK(list, link)	\
  for (link=((JList)(list))->end->prev;		\
       link!=((JList)(list))->end;		\
       link=link->prev)

#define JI_LIST_FOR_EACH_SAFE(list, link, next)			\
  for (link=((JList)(list))->end->next, next=link->next;	\
       link!=((JList)(list))->end;				\
       link=next, next=link->next)

JI_END_DECLS

#endif /* JINETE_LIST_H */
