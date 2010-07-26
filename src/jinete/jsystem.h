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

#ifndef JINETE_JSYSTEM_H_INCLUDED
#define JINETE_JSYSTEM_H_INCLUDED

#include "jinete/jbase.h"

struct BITMAP;

/***********************************************************************/
/* screen related */

#define JI_SCREEN_W ji_screen_w
#define JI_SCREEN_H ji_screen_h

extern struct BITMAP *ji_screen;
extern JRegion ji_dirty_region;
extern int ji_screen_w;
extern int ji_screen_h;

void ji_set_screen(BITMAP *bmp, int width, int height);

void ji_add_dirty_rect(JRect rect);
void ji_add_dirty_region(JRegion region);
void ji_flip_dirty_region();
void ji_flip_rect(JRect rect);

/***********************************************************************/
/* strings related */

void ji_set_translation_hook(const char *(*gettext) (const char *msgid));
const char *ji_translate_string(const char *msgid);

/***********************************************************************/
/* timer related */

extern int volatile ji_clock;	/* in milliseconds */

/***********************************************************************/
/* mouse related */

enum {
  JI_CURSOR_NULL,
  JI_CURSOR_NORMAL,
  JI_CURSOR_NORMAL_ADD,
  JI_CURSOR_FORBIDDEN,
  JI_CURSOR_HAND,
  JI_CURSOR_SCROLL,
  JI_CURSOR_MOVE,
  JI_CURSOR_SIZE_TL,
  JI_CURSOR_SIZE_T,
  JI_CURSOR_SIZE_TR,
  JI_CURSOR_SIZE_L,
  JI_CURSOR_SIZE_R,
  JI_CURSOR_SIZE_BL,
  JI_CURSOR_SIZE_B,
  JI_CURSOR_SIZE_BR,
  JI_CURSOR_EYEDROPPER,
  JI_CURSORS
};

int jmouse_get_cursor();
int jmouse_set_cursor(int type);
void jmouse_draw_cursor();

void jmouse_hide();
void jmouse_show();

bool jmouse_is_hidden();
bool jmouse_is_shown();

bool jmouse_poll();
void jmouse_set_position(int x, int y);

int jmouse_b(int antique);
int jmouse_x(int antique);
int jmouse_y(int antique);
int jmouse_z(int antique);

bool jmouse_control_infinite_scroll(JRect rect);

#endif
