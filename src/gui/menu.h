// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_MENU_H_INCLUDED
#define GUI_MENU_H_INCLUDED

#include "gui/base.h"

Widget* jmenu_new();
Widget* jmenubar_new();
Widget* jmenubox_new();
Widget* jmenuitem_new(const char *text);

Widget* jmenubox_get_menu(Widget* menubox);
Widget* jmenubar_get_menu(Widget* menubar);
Widget* jmenuitem_get_submenu(Widget* menuitem);
JAccel jmenuitem_get_accel(Widget* menuitem);
bool jmenuitem_has_submenu_opened(Widget* menuitem);

void jmenubox_set_menu(Widget* menubox, Widget* menu);
void jmenubar_set_menu(Widget* menubar, Widget* menu);
void jmenuitem_set_submenu(Widget* menuitem, Widget* menu);
void jmenuitem_set_accel(Widget* menuitem, JAccel accel);

int jmenuitem_is_highlight(Widget* menuitem);

void jmenu_popup(Widget* menu, int x, int y);

#endif
