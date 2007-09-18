/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_MENU_H
#define JINETE_MENU_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jmenu_new(void);
JWidget jmenubar_new(void);
JWidget jmenubox_new(void);
JWidget jmenuitem_new(const char *text);

JWidget jmenubox_get_menu(JWidget menubox);
JWidget jmenubar_get_menu(JWidget menubar);
JWidget jmenuitem_get_submenu(JWidget menuitem);
JAccel jmenuitem_get_accel(JWidget menuitem);

void jmenubox_set_menu(JWidget menubox, JWidget menu);
void jmenubar_set_menu(JWidget menubar, JWidget menu);
void jmenuitem_set_submenu(JWidget menuitem, JWidget menu);
void jmenuitem_set_accel(JWidget menuitem, JAccel accel);

int jmenuitem_is_highlight(JWidget menuitem);

void jmenu_popup(JWidget menu, int x, int y);

JI_END_DECLS

#endif /* JINETE_MENU_H */
