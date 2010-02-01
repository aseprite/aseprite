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

#ifndef JINETE_JVIEW_H_INCLUDED
#define JINETE_JVIEW_H_INCLUDED

#include "jinete/jbase.h"

JWidget jview_new();

bool jview_has_bars(JWidget view);

void jview_attach(JWidget view, JWidget viewable_widget);
void jview_maxsize(JWidget view);
void jview_without_bars(JWidget view);

void jview_set_size(JWidget view, int w, int h);
void jview_set_scroll(JWidget view, int x, int y);
void jview_get_scroll(JWidget view, int *x, int *y);
void jview_get_max_size(JWidget view, int *w, int *h);

void jview_update(JWidget view);

JWidget jview_get_viewport(JWidget view);
JRect jview_get_viewport_position(JWidget view);

/* for themes */
void jtheme_scrollbar_info(JWidget scrollbar, int *pos, int *len);

/* for viewable widgets */
JWidget jwidget_get_view(JWidget viewable_widget);

#endif
