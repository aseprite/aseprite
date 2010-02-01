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

#ifndef JINETE_JBUTTON_H_INCLUDED
#define JINETE_JBUTTON_H_INCLUDED

#include "jinete/jbase.h"

struct BITMAP;

/******************/
/* generic button */

JWidget ji_generic_button_new(const char *text,
			      int behavior_type,
			      int draw_type);

void ji_generic_button_set_icon(JWidget button, struct BITMAP *icon);
void ji_generic_button_set_icon_align(JWidget button, int icon_align);

struct BITMAP *ji_generic_button_get_icon(JWidget button);
int ji_generic_button_get_icon_align(JWidget button);

/*****************/
/* normal button */

JWidget jbutton_new(const char *text);

void jbutton_set_bevel(JWidget button, int b0, int b1, int b2, int b3);
void jbutton_get_bevel(JWidget button, int *b4);

void jbutton_add_command(JWidget button, void (*command)(JWidget button));
void jbutton_add_command_data(JWidget button,
			      void (*command)(JWidget button, void *data),
			      void *data);

/****************/
/* check button */

JWidget jcheck_new(const char *text);

/****************/
/* radio button */

JWidget jradio_new(const char *text, int radio_group);

void jradio_set_group(JWidget radio, int radio_group);
int jradio_get_group(JWidget radio);
void jradio_deselect_group(JWidget radio);

#endif
