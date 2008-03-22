/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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
 *   * Neither the name of the Jinete nor the names of its contributors may
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

#ifndef JINETE_MANAGER_H
#define JINETE_MANAGER_H

#include "jinete/jbase.h"

JI_BEGIN_DECLS

JWidget ji_get_default_manager(void);

JWidget jmanager_new(void);
void jmanager_free(JWidget manager);

void jmanager_run(JWidget manager);
bool jmanager_generate_messages(JWidget manager);
void jmanager_dispatch_messages(JWidget manager);

/* timers */

int jmanager_add_timer(JWidget widget, int interval);
void jmanager_remove_timer(int timer_id);
void jmanager_start_timer(int timer_id);
void jmanager_stop_timer(int timer_id);
void jmanager_set_timer_interval(int timer_id, int interval);

/* routines that uses the ji_get_default_manager() */

void jmanager_enqueue_message(JMessage msg);

JWidget jmanager_get_focus(void);
JWidget jmanager_get_mouse(void);
JWidget jmanager_get_capture(void);

void jmanager_set_focus(JWidget widget);
void jmanager_set_mouse(JWidget widget);
void jmanager_set_capture(JWidget widget);
void jmanager_attract_focus(JWidget widget);
void jmanager_focus_first_child(JWidget widget);
void jmanager_free_focus(void);
void jmanager_free_mouse(void);
void jmanager_free_capture(void);
void jmanager_free_widget(JWidget widget);
void jmanager_remove_message(JMessage msg);
void jmanager_remove_messages_for(JWidget widget);
void jmanager_refresh_screen(void);

void jmanager_add_msg_filter(int message, JWidget widget);
void jmanager_remove_msg_filter(int message, JWidget widget);
void jmanager_remove_msg_filter_for(JWidget widget);

JI_END_DECLS

#endif /* JINETE_MANAGER_H */
