/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_COMBOBOX_H
#define JINETE_COMBOBOX_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jcombobox_new(void);

void jcombobox_editable(JWidget combobox, bool state);
void jcombobox_clickopen(JWidget combobox, bool state);
void jcombobox_casesensitive(JWidget combobox, bool state);

bool jcombobox_is_editable(JWidget combobox);
bool jcombobox_is_clickopen(JWidget combobox);
bool jcombobox_is_casesensitive(JWidget combobox);

void jcombobox_add_string(JWidget combobox, const char *string);
void jcombobox_del_string(JWidget combobox, const char *string);
void jcombobox_del_index(JWidget combobox, int index);

void jcombobox_select_index(JWidget combobox, int index);
void jcombobox_select_string(JWidget combobox, const char *string);
int jcombobox_get_selected_index(JWidget combobox);
const char *jcombobox_get_selected_string(JWidget combobox);

const char *jcombobox_get_string(JWidget combobox, int index);
int jcombobox_get_index(JWidget combobox, const char *string);
int jcombobox_get_count(JWidget combobox);

JWidget jcombobox_get_entry_widget(JWidget combobox);

JI_END_DECLS

#endif /* JINETE_COMBOBOX_H */
