// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_LISTBOX_H_INCLUDED
#define GUI_LISTBOX_H_INCLUDED

#include "gui/base.h"

JWidget jlistbox_new();
JWidget jlistitem_new(const char *text);

JWidget jlistbox_get_selected_child(JWidget listbox);
int jlistbox_get_selected_index(JWidget listbox);
void jlistbox_select_child(JWidget listbox, JWidget listitem);
void jlistbox_select_index(JWidget listbox, int index);

int jlistbox_get_items_count(JWidget listbox);

void jlistbox_center_scroll(JWidget listbox);

#endif
