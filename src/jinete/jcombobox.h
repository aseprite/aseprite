/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
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

#ifndef JINETE_JCOMBOBOX_H_INCLUDED
#define JINETE_JCOMBOBOX_H_INCLUDED

#include "jinete/jbase.h"

JWidget jcombobox_new();

void jcombobox_editable(JWidget combobox, bool state);
void jcombobox_clickopen(JWidget combobox, bool state);
void jcombobox_casesensitive(JWidget combobox, bool state);

bool jcombobox_is_editable(JWidget combobox);
bool jcombobox_is_clickopen(JWidget combobox);
bool jcombobox_is_casesensitive(JWidget combobox);

void jcombobox_add_string(JWidget combobox, const char *string, void *data);
void jcombobox_del_string(JWidget combobox, const char *string);
void jcombobox_del_index(JWidget combobox, int index);
void jcombobox_clear(JWidget combobox);

void jcombobox_select_index(JWidget combobox, int index);
void jcombobox_select_string(JWidget combobox, const char *string);
int jcombobox_get_selected_index(JWidget combobox);
const char *jcombobox_get_selected_string(JWidget combobox);

const char *jcombobox_get_string(JWidget combobox, int index);
void *jcombobox_get_data(JWidget combobox, int index);
int jcombobox_get_index(JWidget combobox, const char *string);
int jcombobox_get_count(JWidget combobox);

JWidget jcombobox_get_entry_widget(JWidget combobox);
JWidget jcombobox_get_button_widget(JWidget combobox);

#endif
