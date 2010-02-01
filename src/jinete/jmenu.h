/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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

#ifndef JINETE_JMENU_H_INCLUDED
#define JINETE_JMENU_H_INCLUDED

#include "jinete/jbase.h"

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
