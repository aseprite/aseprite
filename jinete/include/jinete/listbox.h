/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_LISTBOX_H
#define JINETE_LISTBOX_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jlistbox_new(void);
JWidget jlistitem_new(const char *text);

void jlistbox_add_string(JWidget listbox, const char *text);
void jlistbox_delete_string(JWidget listbox, int index);
int jlistbox_find_string(JWidget listbox, const char *text);

JWidget jlistbox_get_selected_child(JWidget listbox);
int jlistbox_get_selected_index(JWidget listbox);
void jlistbox_select_child(JWidget listbox, JWidget listitem);
void jlistbox_select_index(JWidget listbox, int index);

void jlistbox_center_scroll(JWidget listbox);

JI_END_DECLS

#endif /* JINETE_LISTBOX_H */
