/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_MANAGER_H
#define JINETE_MANAGER_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget ji_get_default_manager(void);

JWidget jmanager_new(void);
void jmanager_free(JWidget manager);

void jmanager_run(JWidget manager);
bool jmanager_poll(JWidget manager, bool all_windows);

/* routines that uses the ji_get_default_manager() */

void jmanager_send_message(const JMessage msg);
void jmanager_dispatch_messages(void);
void jmanager_dispatch_draw_messages(void);

JWidget jmanager_get_focus(void);
JWidget jmanager_get_mouse(void);
JWidget jmanager_get_capture(void);

void jmanager_set_focus(JWidget widget);
void jmanager_set_mouse(JWidget widget);
void jmanager_set_capture(JWidget widget);
void jmanager_attract_focus(JWidget widget);
void jmanager_free_focus(void);
void jmanager_free_mouse(void);
void jmanager_free_capture(void);
void jmanager_free_widget(JWidget widget);
void jmanager_remove_message(JMessage msg);
void jmanager_remove_messages_for(JWidget widget);
void jmanager_refresh_screen(void);

JI_END_DECLS

#endif /* JINETE_MANAGER_H */
