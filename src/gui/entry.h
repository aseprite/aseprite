// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_ENTRY_H_INCLUDED
#define GUI_ENTRY_H_INCLUDED

#include "gui/jbase.h"

JWidget jentry_new(size_t maxsize, const char *format, ...);

void jentry_readonly(JWidget entry, bool state);
void jentry_password(JWidget entry, bool state);
bool jentry_is_password(JWidget entry);
bool jentry_is_readonly(JWidget entry);

void jentry_show_cursor(JWidget entry);
void jentry_hide_cursor(JWidget entry);

void jentry_set_cursor_pos(JWidget entry, int pos);
void jentry_select_text(JWidget entry, int from, int to);
void jentry_deselect_text(JWidget entry);

/* for themes */
void jtheme_entry_info(JWidget entry,
		       int *scroll, int *cursor, int *state,
		       int *selbeg, int *selend);

#endif
