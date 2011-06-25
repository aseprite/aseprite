// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_MANAGER_H_INCLUDED
#define GUI_MANAGER_H_INCLUDED

#include "gui/base.h"

JWidget ji_get_default_manager();

JWidget jmanager_new();
void jmanager_free(JWidget manager);

void jmanager_run(JWidget manager);
bool jmanager_generate_messages(JWidget manager);
void jmanager_dispatch_messages(JWidget manager);

void jmanager_add_to_garbage(Widget* widget);

/* timers */

int jmanager_add_timer(JWidget widget, int interval);
void jmanager_remove_timer(int timer_id);
void jmanager_start_timer(int timer_id);
void jmanager_stop_timer(int timer_id);
void jmanager_set_timer_interval(int timer_id, int interval);
bool jmanager_timer_is_running(int timer_id);

/* routines that uses the ji_get_default_manager() */

void jmanager_enqueue_message(Message* msg);

JWidget jmanager_get_top_window();

JWidget jmanager_get_focus();
JWidget jmanager_get_mouse();
JWidget jmanager_get_capture();

void jmanager_set_focus(JWidget widget);
void jmanager_set_mouse(JWidget widget);
void jmanager_set_capture(JWidget widget);
void jmanager_attract_focus(JWidget widget);
void jmanager_focus_first_child(JWidget widget);
void jmanager_free_focus();
void jmanager_free_mouse();
void jmanager_free_capture();
void jmanager_free_widget(JWidget widget);
void jmanager_remove_message(Message* msg);
void jmanager_remove_messages_for(JWidget widget);
void jmanager_refresh_screen();

void jmanager_add_msg_filter(int message, JWidget widget);
void jmanager_remove_msg_filter(int message, JWidget widget);
void jmanager_remove_msg_filter_for(JWidget widget);

void jmanager_invalidate_region(JWidget widget, JRegion region);

#endif
