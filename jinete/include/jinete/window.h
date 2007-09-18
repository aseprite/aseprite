/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_WINDOW_H
#define JINETE_WINDOW_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jwindow_new(const char *text);
JWidget jwindow_new_desktop(void);

JWidget jwindow_get_killer(JWidget window);
JWidget jwindow_get_manager(JWidget window);

void jwindow_moveable(JWidget window, bool state);
void jwindow_sizeable(JWidget window, bool state);
void jwindow_ontop(JWidget window, bool state);

void jwindow_remap(JWidget window);
void jwindow_center(JWidget window);
void jwindow_position(JWidget window, int x, int y);
void jwindow_move(JWidget window, JRect rect);

void jwindow_open(JWidget window);
void jwindow_open_fg(JWidget window);
void jwindow_open_bg(JWidget window);
void jwindow_close(JWidget window, JWidget killer);

bool jwindow_is_toplevel(JWidget window);
bool jwindow_is_foreground(JWidget window);
bool jwindow_is_desktop(JWidget window);
bool jwindow_is_ontop(JWidget window);

JI_END_DECLS

#endif /* JINETE_WINDOW_H */
