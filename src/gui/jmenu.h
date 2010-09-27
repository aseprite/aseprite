// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JMENU_H_INCLUDED
#define GUI_JMENU_H_INCLUDED

#include "gui/jbase.h"

JWidget jmenu_new();
JWidget jmenubar_new();
JWidget jmenubox_new();
JWidget jmenuitem_new(const char *text);

JWidget jmenubox_get_menu(JWidget menubox);
JWidget jmenubar_get_menu(JWidget menubar);
JWidget jmenuitem_get_submenu(JWidget menuitem);
JAccel jmenuitem_get_accel(JWidget menuitem);
bool jmenuitem_has_submenu_opened(JWidget menuitem);

void jmenubox_set_menu(JWidget menubox, JWidget menu);
void jmenubar_set_menu(JWidget menubar, JWidget menu);
void jmenuitem_set_submenu(JWidget menuitem, JWidget menu);
void jmenuitem_set_accel(JWidget menuitem, JAccel accel);

int jmenuitem_is_highlight(JWidget menuitem);

void jmenu_popup(JWidget menu, int x, int y);

#endif
