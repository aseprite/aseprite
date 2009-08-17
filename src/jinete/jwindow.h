/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
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

#ifndef JINETE_JWINDOW_H_INCLUDED
#define JINETE_JWINDOW_H_INCLUDED

#include "jinete/jbase.h"

JWidget jwindow_new(const char *text);
JWidget jwindow_new_desktop();

JWidget jwindow_get_killer(JWidget window);

void jwindow_moveable(JWidget window, bool state);
void jwindow_sizeable(JWidget window, bool state);
void jwindow_ontop(JWidget window, bool state);
void jwindow_wantfocus(JWidget window, bool state);

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
bool jwindow_is_wantfocus(JWidget window);

#endif
