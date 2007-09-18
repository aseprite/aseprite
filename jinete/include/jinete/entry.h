/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_ENTRY_H
#define JINETE_ENTRY_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jentry_new(int maxsize, const char *format, ...);

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

JI_END_DECLS

#endif /* JINETE_ENTRY_H */
